#include "FreeRTOS.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "task.h"
#include <stdint.h>
#include <stdio.h>

extern void uart_init(void);
extern NetworkInterface_t *pxFillInterfaceDescriptor(BaseType_t xEMACIndex, NetworkInterface_t *pxInterface);

#define MAGIC 0xDEADBEEF
#define MAX_PACKET_SIZE 256
#define STACK_WORDS 1024
#define FLAG_USER 0
#define APPLY_DELAY_MS 800

struct request {
	uint32_t magic;
	uint32_t session_id;
	uint16_t inner_len;
	uint16_t flags;
};

volatile BaseType_t xNetworkUp = pdFALSE;
volatile BaseType_t packet_ready = pdFALSE;
volatile struct request shared;

static StackType_t rx_stack[STACK_WORDS];
static StaticTask_t rx_task;
static StackType_t worker_stack[STACK_WORDS];
static StaticTask_t worker_task;

void
unlock_firmware(void)
{
	printf("========================= FIRMWARE UNLOCKED =========================\n");
	while(1) {
		;
	}
}

static void
vRxTask(void *pvParameters)
{
	(void)pvParameters;

	printf("[*] RX task started. Waiting for network...\n");
	while (xNetworkUp == pdFALSE) {
		vTaskDelay(pdMS_TO_TICKS(100));
	}

	Socket_t sock = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_DGRAM, FREERTOS_IPPROTO_UDP);
	if (sock == FREERTOS_INVALID_SOCKET) {
		printf("[-] Failed to create socket\n");
		vTaskDelete(NULL);
	}

	struct freertos_sockaddr bind_addr = {0};
	bind_addr.sin_family = FREERTOS_AF_INET;
	bind_addr.sin_port = FreeRTOS_htons(8888);
	if (FreeRTOS_bind(sock, &bind_addr, sizeof(bind_addr)) != 0) {
		printf("[-] Failed to bind socket\n");
		vTaskDelete(NULL);
	}

	printf("[*] Socket bound to port 8888. Staging every packet.\n");

	while(1) {
		struct freertos_sockaddr client;
		uint32_t client_len = sizeof(client);
		char payload[MAX_PACKET_SIZE];

		int32_t nbytes = FreeRTOS_recvfrom(sock, payload, sizeof(payload), 0, &client, &client_len);
		if (nbytes < (int32_t)sizeof(struct request)) {
			continue;
		}

		struct request *incoming = (struct request *)payload;
		if (incoming->magic != MAGIC) {
			printf("[-] Invalid magic: %x\n", incoming->magic);
			continue;
		}

		// VULNERABILITY: Publish the latest header with no lock. The worker may already
		// have checked an older copy.
		shared.magic = incoming->magic;
		shared.session_id = incoming->session_id;
		shared.inner_len = incoming->inner_len;
		shared.flags = incoming->flags;
		packet_ready = pdTRUE;
		printf("[*] staged session %u flags %u\n", shared.session_id, shared.flags);
	}
}

static void
vWorkerTask(void *pvParameters)
{
	(void)pvParameters;

	printf("[*] Worker started.\n");

	while(1) {
		if (packet_ready == pdFALSE) {
			vTaskDelay(pdMS_TO_TICKS(20));
			continue;
		}

		uint32_t seen_magic = shared.magic;
		uint16_t seen_flags = shared.flags;
		packet_ready = pdFALSE;

		if (seen_magic != MAGIC || seen_flags != FLAG_USER) {
			printf("[-] rejected flags=%u\n", seen_flags);
			continue;
		}

		printf("[+] check passed, flags=%u. Applying shortly.\n", seen_flags);
		// VULNERABILITY: TOCTOU. seen_flags passed the check above, then this
		// task waits. vRxTask is higher priority and stores the next packet
		// into `shared` during that wait. The apply below reads shared.flags
		// again, so the value that is used is not the value that was checked.
		vTaskDelay(pdMS_TO_TICKS(APPLY_DELAY_MS));

		printf("[*] applying flags=%u\n", shared.flags);
		if (shared.flags != FLAG_USER) {
			unlock_firmware();
		}
		// Fix: apply the snapshot that passed the check, and ignore later writes.
		// printf("[*] applying flags=%u\n", seen_flags);
		// if (seen_flags != FLAG_USER) {
		//     unlock_firmware();
		// }
		printf("[+] applied unprivileged request\n");
	}
}

BaseType_t
xApplicationGetRandomNumber(uint32_t *pulNumber)
{
	*pulNumber = 0x12345678;
	return pdTRUE;
}

// Enable Machine External Interrupts (MIE.MEIE = bit 11).
// Do not enable global interrupts (MSTATUS.MIE) here. FreeRTOS enables them
// when the first task starts.
void
setup_interrupts(void)
{
	__asm__ volatile("csrs mie, %0" ::"r"(1 << 11));
}

int
main(void)
{
	uart_init();
	setvbuf(stdout, NULL, _IONBF, 0);
	printf("\n\n--- FreeRTOS RISC-V Proxy ---\n");

	setup_interrupts();

	static NetworkInterface_t interfaces[1];
	static NetworkEndPoint_t endpoints[1];

	pxFillInterfaceDescriptor(0, &interfaces[0]);

	const uint8_t ip_addr[4] = {configIP_ADDR0, configIP_ADDR1, configIP_ADDR2, configIP_ADDR3};
	const uint8_t netmask[4] = {configNET_MASK0, configNET_MASK1, configNET_MASK2, configNET_MASK3};
	const uint8_t gateway[4] = {configGATEWAY_ADDR0, configGATEWAY_ADDR1, configGATEWAY_ADDR2, configGATEWAY_ADDR3};
	const uint8_t dns_addr[4] = {configGATEWAY_ADDR0, configGATEWAY_ADDR1, configGATEWAY_ADDR2, configGATEWAY_ADDR3};
	const uint8_t mac_addr[6] = {configMAC_ADDR0, configMAC_ADDR1, configMAC_ADDR2, configMAC_ADDR3, configMAC_ADDR4, configMAC_ADDR5};

	FreeRTOS_FillEndPoint(&interfaces[0], &endpoints[0], ip_addr, netmask, gateway, dns_addr, mac_addr);

	printf("[*] Starting IP stack...\n");
	FreeRTOS_IPInit_Multi();
	printf("[*] IP stack started.\n");

	// RX must preempt the worker while a checked request is waiting to be applied.
	xTaskCreateStatic(vRxTask, "RxTask", STACK_WORDS, NULL, 2, rx_stack, &rx_task);
	xTaskCreateStatic(vWorkerTask, "Worker", STACK_WORDS, NULL, 1, worker_stack, &worker_task);

	printf("[*] Starting scheduler...\n");
	vTaskStartScheduler();

	while(1) {
		;
	}
	return 0;
}

void
vApplicationIPNetworkEventHook_Multi(eIPCallbackEvent_t eNetworkEvent, struct xNetworkEndPoint *pxEndPoint)
{
	(void)pxEndPoint;
	if (eNetworkEvent == eNetworkUp) {
		printf("[+] Network is up!\n");
		xNetworkUp = pdTRUE;
	}
}
