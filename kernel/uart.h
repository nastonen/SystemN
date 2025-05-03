#pragma once

#include "spinlock.h"

#define UART0               0x10000000L
#define UART_RHR            0           // Receive Holding Register (read only)
#define UART_LSR            5           // Line Status Register
#define UART_LSR_RX_READY   (1 << 0)    // Bit 0: Receiver data ready

extern spinlock_t uart_lock;

void uart_putc(char c);
void uart_puts(const char* s);
void uart_puthex(ulong x);
void uart_putlong(ulong val);

char uart_getc(void);
int uart_gets(char *buf, int maxlen);
