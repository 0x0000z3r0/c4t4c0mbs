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

struct proxy_header {
	uint32_t magic;
	uint32_t session_id;
	uint16_t inner_len;
	uint16_t flags;
};

volatile BaseType_t xNetworkUp = pdFALSE;

void
unlock_firmware(void)
{
	printf("========================= FIRMWARE UNLOCKED =========================\n");
	// In a real RTOS, we might halt or reset here.
	while(1);
}

void
vProxyTask(void *pvParameters)
{
	(void)pvParameters;

	printf("[*] Proxy task started. Waiting for network...\n");

	while (xNetworkUp == pdFALSE) {
		vTaskDelay(pdMS_TO_TICKS(100));
	}

	printf("[*] Network is up. Creating socket...\n");

	Socket_t xSocket = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_DGRAM, FREERTOS_IPPROTO_UDP);
	if (xSocket == FREERTOS_INVALID_SOCKET) {
		printf("[-] Failed to create socket\n");
		vTaskDelete(NULL);
	}

	printf("[*] Socket created. Binding to port 8888...\n");

	struct freertos_sockaddr xBindAddress = {0};
	xBindAddress.sin_family = FREERTOS_AF_INET;
	xBindAddress.sin_port = FreeRTOS_htons(8888);
	if (FreeRTOS_bind(xSocket, &xBindAddress, sizeof(xBindAddress)) != 0) {
		printf("[-] Failed to bind socket\n");
		vTaskDelete(NULL);
	}

	printf("[*] Socket bound to port 8888. Listening for packets...\n");

	while(1) {
		struct freertos_sockaddr xClient;
		uint32_t xClientLength = sizeof(xClient);

		// VULNERABILITY: Stack Buffer Overflow
		// We allocate a fixed size buffer on the stack but tell recvfrom
		// it can write up to 1024 bytes.
		char payload[MAX_PACKET_SIZE];

		// Wait for a packet.
		int32_t lBytes = FreeRTOS_recvfrom(xSocket, payload, 1024, 0, &xClient, &xClientLength);
		if (lBytes <= 0) {
			continue;
		}

		printf("[*] Received %d bytes from UDP\n", (int)lBytes);

		if (lBytes < (int32_t)sizeof(struct proxy_header)) {
			continue;
		}

		struct proxy_header *header = (struct proxy_header *)payload;

		if (header->magic != MAGIC) {
			printf("[-] Invalid magic: %x\n", header->magic);
			continue;
		}

		printf("[+] Receiving payload of %d bytes for session %d...\n", header->inner_len, header->session_id);
		printf("[+] Payload processed successfully.\n");
		// Return so the overflowed saved ra is loaded.
		return;
	}
}

// Random number generator for the TCP/IP stack.
BaseType_t
xApplicationGetRandomNumber(uint32_t *pulNumber)
{
	*pulNumber = 0x12345678;
	return pdTRUE;
}

// Enable Machine External Interrupts (MIE.MEIE = bit 11).
// Do not enable global interrupts (MSTATUS.MIE) here. FreeRTOS enables them
// when the first task starts. Enabling them earlier lets a pending UART
// interrupt run before pxCurrentTCB is initialized.
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

	static NetworkInterface_t xInterfaces[1];
	static NetworkEndPoint_t xEndPoints[1];

	pxFillInterfaceDescriptor(0, &xInterfaces[0]);

	const uint8_t ucIPAddress[4] = {configIP_ADDR0, configIP_ADDR1, configIP_ADDR2, configIP_ADDR3};
	const uint8_t ucNetMask[4] = {configNET_MASK0, configNET_MASK1, configNET_MASK2, configNET_MASK3};
	const uint8_t ucGatewayAddress[4] = {configGATEWAY_ADDR0, configGATEWAY_ADDR1, configGATEWAY_ADDR2, configGATEWAY_ADDR3};
	const uint8_t ucDNSServerAddress[4] = {configGATEWAY_ADDR0, configGATEWAY_ADDR1, configGATEWAY_ADDR2, configGATEWAY_ADDR3};
	const uint8_t ucMACAddress[6] = {configMAC_ADDR0, configMAC_ADDR1, configMAC_ADDR2, configMAC_ADDR3, configMAC_ADDR4, configMAC_ADDR5};

	FreeRTOS_FillEndPoint(&xInterfaces[0], &xEndPoints[0], ucIPAddress, ucNetMask, ucGatewayAddress, ucDNSServerAddress, ucMACAddress);

	printf("[*] Starting IP stack...\n");
	FreeRTOS_IPInit_Multi();
	printf("[*] IP stack started.\n");

	printf("[*] Creating proxy task...\n");
	xTaskCreate(vProxyTask, "ProxyTask", 1024, NULL, 1, NULL);
	printf("[*] Proxy task created.\n");

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
