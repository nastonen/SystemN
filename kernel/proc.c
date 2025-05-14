#include "proc.h"
#include "uart.h"
#include "mm/snub.h"
#include "list.h"

static long next_pid = 1;

cpu_t cpus[NCPU];
proc_t idle_procs[NCPU];

void
idle_loop()
{
    DEBUG_PRINT(
        uart_puts("Hart ");
        uart_putc('0' + curr_cpu()->id);
        uart_puts(": idle loop\n");
    );

    // Enable interrupts
    set_csr(sstatus, SSTATUS_SIE);

    while (1)
        asm volatile("wfi");  // Wait for interrupt
}

proc_t *
create_proc()
{
    proc_t *p = (proc_t *)kzalloc(sizeof(proc_t));
    if (!p) {
        DEBUG_PRINT(
            uart_puts("Process creation failed. Return NULL.\n");
        );

        return NULL;
    }

    p->pid = ATOMIC_FETCH_AND_INC(&next_pid);
    p->bound_cpu = -1;
    p->tf = (trap_frame_t *)(p->kstack + KSTACK_SIZE - sizeof(trap_frame_t));

    // Put into RUNNABLE queue
    p->state = RUNNABLE;
    list_add_tail(&p->q_node, &cpus[curr_cpu()->id].run_queue);

    return p;
}

void
free_proc(proc_t *p)
{
    if (!p)
        return;

    // If it's in a queue, remove it
    if (list_in_queue(&p->q_node))
        list_del(&p->q_node);

    kfree(p);
}
