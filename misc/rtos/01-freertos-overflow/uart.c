#include "FreeRTOS.h"
#include "task.h"
#include <stdint.h>
#include <stdio.h>

#define UART0_BASE 0x10000000UL

// QEMU 6.2 virt has only one MMIO 16550 (UART0). A second -serial flag does
// not create UART1 at 0x10000100 — that address is unmapped and faults.
// UART1 is a pci-serial 16550, mapped into the virt PCI IO window.
#define PCI_ECAM_BASE 0x30000000UL
#define PCI_PIO_BASE 0x03000000UL
#define PCI_IO_PORT 0x100
#define PCI_VENDOR_REDHAT 0x1b36
#define PCI_DEVICE_SERIAL 0x0002

#define UART_RHR 0  // Receive Holding Register (read mode)
#define UART_THR 0  // Transmit Holding Register (write mode)
#define UART_IER 1  // Interrupt Enable Register
#define UART_FCR 2  // FIFO Control Register (write mode)
#define UART_LCR 3  // Line Control Register
#define UART_LSR 5  // Line Status Register

#define UART_LSR_DR 0x01   // Data ready
#define UART_LSR_THRE 0x20 // Transmit-hold-register empty

static volatile uint8_t *uart1_base = NULL;

static void
uart16550_init(volatile uint8_t *uart)
{
	uart[UART_IER] = 0x00; // Disable all interrupts; RX is polled
	uart[UART_LCR] = 0x80; // Enable DLAB (set baud rate divisor)
	uart[UART_THR] = 0x03; // Divisor latch (LSB)
	uart[UART_IER] = 0x00; // Divisor latch (MSB)
	uart[UART_LCR] = 0x03; // 8 bits, no parity, one stop bit
	uart[UART_FCR] = 0x07; // Enable FIFO, clear them
}

static uint32_t
pci_cfg_read32(unsigned dev, unsigned offset)
{
	volatile uint32_t *reg = (volatile uint32_t *)(PCI_ECAM_BASE + (dev << 15) + offset);
	return *reg;
}

static void
pci_cfg_write32(unsigned dev, unsigned offset, uint32_t value)
{
	volatile uint32_t *reg = (volatile uint32_t *)(PCI_ECAM_BASE + (dev << 15) + offset);
	*reg = value;
}

static int
map_pci_uart1(void)
{
	for (unsigned dev = 0; dev < 32; dev++) {
		uint32_t pci_id = pci_cfg_read32(dev, 0);
		if ((pci_id & 0xFFFFu) != PCI_VENDOR_REDHAT) {
			continue;
		}
		if ((pci_id >> 16) != PCI_DEVICE_SERIAL) {
			continue;
		}

		// BAR0 is an 8-byte IO BAR. Identity-map it into virt PIO.
		pci_cfg_write32(dev, 0x10, PCI_IO_PORT | 0x1u);
		uint32_t cmd = pci_cfg_read32(dev, 0x04);
		pci_cfg_write32(dev, 0x04, cmd | 0x1u); // IO space enable

		uart1_base = (volatile uint8_t *)(PCI_PIO_BASE + PCI_IO_PORT);
		uart16550_init(uart1_base);
		return 1;
	}
	return 0;
}

void
uart_init(void)
{
	volatile uint8_t *uart0 = (volatile uint8_t *)UART0_BASE;
	uart16550_init(uart0);
	map_pci_uart1();
}

void
uart_putc(char byte)
{
	volatile uint8_t *uart0 = (volatile uint8_t *)UART0_BASE;
	while ((uart0[UART_LSR] & UART_LSR_THRE) == 0) {
		;
	}
	uart0[UART_THR] = byte;
}

void
uart_putc_net(char byte)
{
	if (uart1_base == NULL) {
		return;
	}
	while ((uart1_base[UART_LSR] & UART_LSR_THRE) == 0) {
		;
	}
	uart1_base[UART_THR] = byte;
}

char
uart_getc(void)
{
	if (uart1_base == NULL) {
		while(1) {
			vTaskDelay(pdMS_TO_TICKS(1000));
		}
	}

	while(1) {
		if (uart1_base[UART_LSR] & UART_LSR_DR) {
			return (char)uart1_base[UART_RHR];
		}
		// RX task is high priority; must block so the IP stack can run.
		vTaskDelay(1);
	}
}

static int
uart_putc_stdio(char byte, FILE *file)
{
	(void)file;
	if (byte == '\n') {
		uart_putc('\r');
	}
	uart_putc(byte);
	return byte;
}

static FILE __stdio = FDEV_SETUP_STREAM(uart_putc_stdio, NULL, NULL, _FDEV_SETUP_WRITE);
FILE *const stdout = &__stdio;

void
freertos_risc_v_application_interrupt_handler(void)
{
	// UART1 is polled. Claim and complete any unexpected PLIC IRQ.
	uint32_t cause;
	__asm__ volatile("csrr %0, mcause"
			 : "=r"(cause));

	if ((cause & 0x7FFFFFFF) == 11) {
		volatile uint32_t *claim = (volatile uint32_t *)0x0c200004UL;
		uint32_t irq = *claim;
		if (irq) {
			*claim = irq;
		}
	}
}
