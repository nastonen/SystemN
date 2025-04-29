#pragma once

#include "spinlock.h"

extern spinlock_t uart_lock;

void uart_putc(char c);
void uart_puts(const char* s);
void uart_puthex(ulong x);

char uart_getc(void);
int uart_gets(char *buf, int maxlen);
