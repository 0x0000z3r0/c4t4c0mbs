FREERTOS_DIR = ../FreeRTOS-Kernel
FREERTOS_TCP_DIR = ../FreeRTOS-Plus-TCP

CC = riscv64-unknown-elf-gcc
OBJCOPY = riscv64-unknown-elf-objcopy

CFLAGS = -march=rv64imac -mabi=lp64 -mcmodel=medany \
         -Wall -Wextra -g3 -O0 \
         --specs=picolibc.specs \
         -I$(FREERTOS_DIR)/include \
         -I$(FREERTOS_DIR)/portable/GCC/RISC-V \
         -I$(FREERTOS_TCP_DIR)/source/include \
         -I$(FREERTOS_TCP_DIR)/source/portable/Compiler/GCC \
         -I.

LDFLAGS = -T fake_rom.ld -nostartfiles -Wl,--gc-sections

SRCS = main.c \
       uart.c \
       NetworkInterface.c \
       $(FREERTOS_DIR)/tasks.c \
       $(FREERTOS_DIR)/queue.c \
       $(FREERTOS_DIR)/list.c \
       $(FREERTOS_DIR)/timers.c \
       $(FREERTOS_DIR)/event_groups.c \
       $(FREERTOS_DIR)/portable/GCC/RISC-V/port.c \
       $(FREERTOS_DIR)/portable/MemMang/heap_4.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_ARP.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_BitConfig.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_DHCP.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_DHCPv6.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_DNS.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_DNS_Cache.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_DNS_Callback.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_DNS_Networking.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_DNS_Parser.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_ICMP.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_IP.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_IP_Timers.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_IP_Utils.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_IPv4.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_IPv4_Sockets.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_IPv4_Utils.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_IPv6.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_IPv6_Sockets.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_IPv6_Utils.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_ND.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_RA.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_Routing.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_Sockets.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_Stream_Buffer.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_TCP_IP.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_TCP_IP_IPv4.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_TCP_IP_IPv6.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_TCP_Reception.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_TCP_State_Handling.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_TCP_State_Handling_IPv4.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_TCP_State_Handling_IPv6.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_TCP_Transmission.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_TCP_Transmission_IPv4.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_TCP_Transmission_IPv6.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_TCP_Utils.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_TCP_Utils_IPv4.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_TCP_Utils_IPv6.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_TCP_WIN.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_Tiny_TCP.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_UDP_IP.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_UDP_IPv4.c \
       $(FREERTOS_TCP_DIR)/source/FreeRTOS_UDP_IPv6.c \
       $(FREERTOS_TCP_DIR)/source/portable/BufferManagement/BufferAllocation_2.c

ASM_SRCS = start.S \
           $(FREERTOS_DIR)/portable/GCC/RISC-V/portASM.S

OBJS = $(SRCS:.c=.o) $(ASM_SRCS:.S=.o)

TARGET = rtos_proxy.elf

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	qemu-system-riscv64 -machine virt -bios none -display none \
		-serial stdio \
		-chardev socket,id=netuart,host=127.0.0.1,port=8888,server=on,wait=off \
		-device pci-serial,chardev=netuart \
		-kernel $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all run clean
