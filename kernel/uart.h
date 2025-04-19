#pragma once

#include "spinlock.h"

extern spinlock_t uart_lock;

void uart_putc(char c);
void uart_puts(const char* s);
void uart_puthex(ulong x);
