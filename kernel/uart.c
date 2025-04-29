#include "uart.h"

#define UART0 0x10000000L

spinlock_t uart_lock = SPINLOCK_INIT;

void
uart_putc(char c)
{
    *(volatile char *)UART0 = c;
}

void
uart_puts(const char *s)
{
    while (*s)
        uart_putc(*s++);
}

void
uart_puthex(ulong x)
{
    uart_puts("0x");
    for (int i = (sizeof(x) * 2) - 1; i >= 0; i--) {
        int nibble = (x >> (i * 4)) & 0xf;
        uart_putc(nibble < 10 ? '0' + nibble : 'a' + nibble - 10);
    }
}

#define UART0 0x10000000L
#define UART_RHR 0   // Receive Holding Register (read only)
#define UART_LSR 5   // Line Status Register
#define UART_LSR_RX_READY (1 << 0)  // Bit 0: Receiver data ready

char uart_getc(void)
{
    volatile char *uart = (volatile char *)UART0;

    /*
    while (1) {
        if (uart[UART_LSR] & UART_LSR_RX_READY) {
            return uart[UART_RHR];  // return valid char
        }

        // Give up CPU while waiting — cooperative multitasking
        syscall0(SYS_yield);
    }
    */

    // Wait until data is available in receive buffer
    while ((uart[UART_LSR] & UART_LSR_RX_READY) == 0)
        ;

    return uart[UART_RHR];
}

/*
char
uart_getc(void)
{
    volatile char *uart = (volatile char *)UART0;
    char c;

    // Busy wait until UART has received something
    // Might need a status register to check for input ready
    do {
        c = *uart;
    } while (c == 0); // Or maybe 0xff depending on UART

    return c;
}
*/

int
uart_gets(char *buf, int maxlen)
{
    int i = 0;
    while (i < maxlen - 1) {
        char c = uart_getc();
        if (c == '\r') // Enter key is usually CR
            c = '\n';
        uart_putc(c); // Echo back

        if (c == '\n') {
            buf[i++] = c;
            break;
        }
        buf[i++] = c;
    }
    buf[i] = 0; // Null-terminate

    return i;
}
