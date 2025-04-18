#include "uart.h"
#include "spinlock.h"

unsigned long read_mhartid();

static spinlock_t uart_lock = SPINLOCK_INIT;

void
start()
{
    spin_lock(&uart_lock);

    uart_puts("Hart ");
    uart_putc('0' + read_mhartid());
    uart_puts(": Hello from RISC-V!\n");

    spin_unlock(&uart_lock);
}

unsigned long
read_mhartid()
{
    unsigned long id;
    asm volatile ("csrr %0, mhartid" : "=r"(id));

    return id;
}
