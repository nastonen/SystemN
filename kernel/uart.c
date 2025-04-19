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
