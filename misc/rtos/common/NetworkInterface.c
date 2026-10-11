#include "FreeRTOS.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Routing.h"
#include "NetworkBufferManagement.h"
#include "task.h"
#include <stdint.h>

extern char uart_getc(void);
extern void uart_putc_net(char byte);

#define SLIP_END 0xC0
#define SLIP_ESC 0xDB
#define SLIP_ESC_END 0xDC
#define SLIP_ESC_ESC 0xDD

static TaskHandle_t xRxTaskHandle = NULL;
static NetworkInterface_t *pxMyInterface = NULL;

static void
prvRxTask(void *pvParameters)
{
	NetworkBufferDescriptor_t *pxBufferDescriptor = NULL;
	size_t xBytesReceived = 0;
	uint8_t cByte;
	BaseType_t xEscape = pdFALSE;

	(void)pvParameters;

	while(1) {
		cByte = uart_getc();

		if (cByte == SLIP_END) {
			if (xBytesReceived > 0) {
				// Complete frame.
				if (pxBufferDescriptor != NULL) {
					pxBufferDescriptor->xDataLength = xBytesReceived;
					pxBufferDescriptor->pxInterface = pxMyInterface;

					printf("[*] UART received %d bytes\n", (int)xBytesReceived);

					pxBufferDescriptor->pxEndPoint = FreeRTOS_MatchingEndpoint(pxMyInterface, pxBufferDescriptor->pucEthernetBuffer);

					if (pxBufferDescriptor->pxEndPoint == NULL) {
						printf("[-] No matching endpoint for packet!\n");
						// Network not fully up or unknown endpoint, drop packet.
						vReleaseNetworkBufferAndDescriptor(pxBufferDescriptor);
						pxBufferDescriptor = NULL;
						xBytesReceived = 0;
						xEscape = pdFALSE;
						continue;
					}

					IPStackEvent_t xRxEvent;
					xRxEvent.eEventType = eNetworkRxEvent;
					xRxEvent.pvData = (void *)pxBufferDescriptor;

					if (xSendEventStructToIPTask(&xRxEvent, 0) == pdFALSE) {
						vReleaseNetworkBufferAndDescriptor(pxBufferDescriptor);
					}

					pxBufferDescriptor = NULL;
				}
				xBytesReceived = 0;
			}
			xEscape = pdFALSE;
		} else if (cByte == SLIP_ESC) {
			xEscape = pdTRUE;
		} else {
			if (xEscape) {
				if (cByte == SLIP_ESC_END) {
					cByte = SLIP_END;
				} else if (cByte == SLIP_ESC_ESC) {
					cByte = SLIP_ESC;
				}
				xEscape = pdFALSE;
			}

			if (pxBufferDescriptor == NULL) {
				pxBufferDescriptor = pxGetNetworkBufferWithDescriptor(ipconfigNETWORK_MTU, 0);
			}

			// Packet too large or no buffer: drop bytes until the next END.
			if (pxBufferDescriptor != NULL && xBytesReceived < ipconfigNETWORK_MTU) {
				pxBufferDescriptor->pucEthernetBuffer[xBytesReceived++] = cByte;
			}
		}
	}
}

BaseType_t
xNetworkInterfaceInitialise(struct xNetworkInterface *pxInterface)
{
	pxMyInterface = pxInterface;

	if (xRxTaskHandle == NULL) {
		xTaskCreate(prvRxTask, "NetRx", configMINIMAL_STACK_SIZE * 2, NULL, configMAX_PRIORITIES - 1, &xRxTaskHandle);
	}
	return pdTRUE;
}

BaseType_t
xNetworkInterfaceOutput(struct xNetworkInterface *pxInterface, NetworkBufferDescriptor_t *const pxDescriptor, BaseType_t xReleaseAfterSend)
{
	(void)pxInterface;

	// Send the Ethernet frame over UART1 using SLIP framing.
	uart_putc_net(SLIP_END);

	for (size_t index = 0; index < pxDescriptor->xDataLength; index++) {
		uint8_t cByte = pxDescriptor->pucEthernetBuffer[index];
		if (cByte == SLIP_END) {
			uart_putc_net(SLIP_ESC);
			uart_putc_net(SLIP_ESC_END);
		} else if (cByte == SLIP_ESC) {
			uart_putc_net(SLIP_ESC);
			uart_putc_net(SLIP_ESC_ESC);
		} else {
			uart_putc_net(cByte);
		}
	}

	uart_putc_net(SLIP_END);

	if (xReleaseAfterSend != pdFALSE) {
		vReleaseNetworkBufferAndDescriptor(pxDescriptor);
	}

	return pdTRUE;
}

BaseType_t
xNetworkInterfaceGetPhyLinkStatus(struct xNetworkInterface *pxInterface)
{
	(void)pxInterface;
	return pdTRUE;
}

NetworkInterface_t *
pxFillInterfaceDescriptor(BaseType_t xEMACIndex, NetworkInterface_t *pxInterface)
{
	(void)xEMACIndex;
	static char pcName[17] = "UART";

	if (pxInterface != NULL) {
		pxInterface->pcName = pcName;
		pxInterface->pvArgument = NULL;
		pxInterface->pfInitialise = xNetworkInterfaceInitialise;
		pxInterface->pfOutput = xNetworkInterfaceOutput;
		pxInterface->pfGetPhyLinkStatus = xNetworkInterfaceGetPhyLinkStatus;

		FreeRTOS_AddNetworkInterface(pxInterface);
	}
	return pxInterface;
}
