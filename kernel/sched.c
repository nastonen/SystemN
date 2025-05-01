#include "sched.h"
#include "proc.h"
#include "uart.h"

spinlock_t sched_lock = SPINLOCK_INIT;

// Forward declaration:
void swtch(context_t *old, context_t *new);

void
schedule()
{
    struct cpu *c = curr_cpu();
    struct proc *old = c->proc;
    struct proc *new = (void *)0;

    spin_lock(&sched_lock);
    for (int i = 0; i < NPROC; i++) {
        struct proc *p = &proc_table[i];
        if (p->state == RUNNABLE) {
            // Called yield(), does not want to run
            if (p == old)
                continue;

            // Not my process
            if (p->bound_cpu != -1 && p->bound_cpu != (int)c->id)
                continue;

            new = p;
            break;
        }
    }
    spin_unlock(&sched_lock);

    // No runnable process, run idle
    if (!new && !old->is_idle)
        new = &idle_procs[c->id];

    if (new && new != old) {
        // Mark old proc as not running
        if (!old->is_idle && old->state == RUNNING)
            old->state = RUNNABLE;

        new->state = RUNNING;
        c->proc = new;

        if (new->is_idle) {
            spin_lock(&uart_lock);
            uart_puts("CPU ");
            uart_putc('0' + c->id);
            uart_puts(" switching to idle\n");
            spin_unlock(&uart_lock);
        } else {
            spin_lock(&uart_lock);
            uart_puts("new proc ");
            uart_putc('0' + new->pid);
            uart_puts(" found, switching to user code at ");
            uart_puthex(new->tf->sepc);
            uart_putc('\n');
            spin_unlock(&uart_lock);
        }

        swtch(&old->ctx, &new->ctx);
    } else {
        spin_lock(&uart_lock);
        uart_puts("Staying idle...\n");
        spin_unlock(&uart_lock);
        // Stay in idle
        while (1)
            asm volatile("wfi");
    }
}
