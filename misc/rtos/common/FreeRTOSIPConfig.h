#ifndef FREERTOS_IP_CONFIG_H
#define FREERTOS_IP_CONFIG_H

#define ipconfigBYTE_ORDER pdFREERTOS_LITTLE_ENDIAN

/* Network MAC address */
#define configMAC_ADDR0 0x00
#define configMAC_ADDR1 0x11
#define configMAC_ADDR2 0x22
#define configMAC_ADDR3 0x33
#define configMAC_ADDR4 0x44
#define configMAC_ADDR5 0x55

/* Default IP address */
#define configIP_ADDR0 192
#define configIP_ADDR1 168
#define configIP_ADDR2 0
#define configIP_ADDR3 2

/* Default gateway */
#define configGATEWAY_ADDR0 192
#define configGATEWAY_ADDR1 168
#define configGATEWAY_ADDR2 0
#define configGATEWAY_ADDR3 1

/* Default netmask */
#define configNET_MASK0 255
#define configNET_MASK1 255
#define configNET_MASK2 255
#define configNET_MASK3 0

/* Use static IP address (DHCP disabled) */
#define ipconfigUSE_DHCP 0

/* Advanced configuration */
#define ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS 10
#define ipconfigEVENT_QUEUE_LENGTH 20
#define ipconfigIP_TASK_PRIORITY (configMAX_PRIORITIES - 2)
#define ipconfigIP_TASK_STACK_SIZE_WORDS (configMINIMAL_STACK_SIZE * 5)

/* UDP configuration */
#define ipconfigUSE_UDP 1

/* TCP configuration */
#define ipconfigUSE_TCP 0

/* Network MTU */
#define ipconfigNETWORK_MTU 1500

/* Enable callbacks */
#define ipconfigUSE_NETWORK_EVENT_HOOK 1

/* Buffer allocation scheme */
#define ipconfigUSE_LINKED_RX_MESSAGES 0

/* Enable IPv4 */
#define ipconfigUSE_IPv4 1
#define ipconfigIPv4_BACKWARD_COMPATIBLE 0

#endif /* FREERTOS_IP_CONFIG_H */
