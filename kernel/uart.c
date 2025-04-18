#include "uart.h"

#define UART0 0x10000000L

void
uart_putc(char c)
{
    *(volatile char *)UART0 = c;
}

void
uart_puts(const char *s)
{
    while (*s) uart_putc(*s++);
}
