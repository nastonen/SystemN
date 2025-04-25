#include "proc.h"

struct cpu cpus[NCPU];
struct proc idle_procs[NCPU];

void
idle_loop()
{
    while (1) {
        /*
        spin_lock(&uart_lock);
        uart_puts("Hart ");
        uart_putc('0' + curr_cpu()->id);
        uart_puts(": idle loop\n");
        spin_unlock(&uart_lock);
        */

        asm volatile("wfi");  // Wait for interrupt
    }
}
