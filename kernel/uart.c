#include "uart.h"

ulong UART0 = 0x10000000;
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

ulong
uart_putsn(const char *s, ulong len)
{
    ulong written = 0;
    while (len-- && *s) {
        uart_putc(*s++);
        written++;
    }

    return written;
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

void
uart_putlong(ulong val)
{
    char buf[21]; // Max for 64-bit decimal is 20 digits + null
    int i = 20;
    buf[i--] = '\0';

    if (val == 0) {
        uart_putc('0');
        return;
    }

    while (val > 0 && i >= 0) {
        buf[i--] = '0' + (val % 10);
        val /= 10;
    }

    uart_puts(&buf[i + 1]);
}

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
