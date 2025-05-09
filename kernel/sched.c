#include "sched.h"
#include "proc.h"
#include "uart.h"
#include "list.h"

spinlock_t sched_lock = SPINLOCK_INIT;

// Forward declaration
void swtch(context_t *old, context_t *new);

void
schedule()
{
    spin_lock(&uart_lock);
    uart_puts("schedule() run...\n");
    spin_unlock(&uart_lock);

    cpu_t *c = curr_cpu();
    proc_t *old = c->proc;
    proc_t *new = NULL;

    list_node_t *head = &c->run_queue;
    list_node_t *next_node = old->q_node.next;

    // Check if old is in the run queue
    if (old->state == RUNNABLE && list_in_queue(&old->q_node)) {
        next_node = old->q_node.next;
        if (next_node == head)
            next_node = next_node->next;
    } else {
        // Fallback: pick first in queue
        next_node = head->next;
    }

    if (next_node != head)
        new = container_of(next_node, proc_t, q_node);

    if (new == old)
        new = NULL;


    // No runnable process, run idle
    if (!new && !old->is_idle)
        new = &idle_procs[c->id];

    if (new && new != old) {
        // Mark old proc as not running
        if (old->pid > 0 && old->state == RUNNING) {
            old->state = RUNNABLE;
            list_add_tail(&old->q_node, &c->run_queue);
        }

        // Remove new from run_queue and set RUNNING
        if (list_in_queue(&new->q_node))
            list_del(&new->q_node);
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

        /*
        spin_lock(&uart_lock);
        uart_puts("old->ctx.ra = "); uart_puthex(old->ctx.ra);
        uart_puts("\nold->ctx.sp = "); uart_puthex(old->ctx.sp);
        uart_putc('\n');
        uart_puts("\nnew->ctx.ra = "); uart_puthex(new->ctx.ra);
        uart_puts("\nnew->ctx.sp = "); uart_puthex(new->ctx.sp);
        uart_putc('\n');
        spin_unlock(&uart_lock);
        */

        swtch(&old->ctx, &new->ctx);
    } else {
        spin_lock(&uart_lock);
        uart_puts("CPU ");
        uart_putc('0' + c->id);
        uart_puts(" staying idle...\n");
        spin_unlock(&uart_lock);
        // Stay in idle
        while (1)
            asm volatile("wfi");
    }
}
