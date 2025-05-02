#include "proc.h"
#include "uart.h"

struct cpu cpus[NCPU];
struct proc proc_table[NPROC];

struct proc boot_procs[NCPU];
char boot_stack[NCPU][KSTACK_SIZE];

struct proc idle_procs[NCPU];
char idle_stack[NCPU][KSTACK_SIZE];

void
idle_loop()
{
    spin_lock(&uart_lock);
    uart_puts("Hart ");
    uart_putc('0' + curr_cpu()->id);
    uart_puts(": idle loop\n");
    spin_unlock(&uart_lock);

    while (1)
        asm volatile("wfi");  // Wait for interrupt
}
