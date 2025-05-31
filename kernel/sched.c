#include "sched.h"
#include "proc.h"
#include "uart.h"
#include "list.h"
#include "mm/mem.h"

// Forward declaration
void swtch(context_t *old, context_t *new, pte_t *pagetable);

void
schedule()
{
    cpu_t *c = curr_cpu();
    proc_t *old = c->proc;
    proc_t *new = NULL;

    DEBUG_PRINT(
        uart_putc('0' + c->id);
        uart_puts(" schedule() run...\n");
    );

    list_node_t *head = &c->run_queue;
    list_node_t *node = head->next;

    // Next process found
    if (node != head) {
        new = container_of(node, proc_t, q_node);
        list_del(&new->q_node);
        new->state = RUNNING;

        DEBUG_PRINT(
            uart_puts("new proc ");
            uart_putc('0' + new->pid);
            uart_puts(" found, switching to user code at ");
            uart_puthex(new->tf->sepc);
            uart_putc('\n');
        );

        if (!old->is_idle) {
            old->state = RUNNABLE;
            list_add_tail(&old->q_node, &c->run_queue);
        }
        c->proc = new;

        //load_pagetable(c->proc->pagetable);
        swtch(&old->ctx, &c->proc->ctx, c->proc->pagetable);
    } else if (!old->is_idle) {
        // No runnable process, run idle
        DEBUG_PRINT(
            uart_putc('0' + c->id);
            uart_puts(" switching to idle\n");
        );

        c->proc = &idle_procs[c->id];

        //load_pagetable(kernel_pagetable);
        swtch(&old->ctx, &c->proc->ctx, kernel_pagetable);
    }
}
