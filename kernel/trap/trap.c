#include "trap.h"
#include "../riscv.h"
#include "../uart.h"
#include "../types.h"
#include "../proc.h"
#include "../timer.h"
#include "../syscall.h"
#include "../user_syscall.h"
#include "../string.h"
#include "../sched.h"

void
syscall_handler(proc_t *p)
{
    cpu_t *c = curr_cpu();
    int syscall_num = p->tf->regs[17]; // a7 (syscall number)

    switch (syscall_num) {
    case SYS_write: {
        DEBUG_PRINT(uart_puts("SYS_write\n"););
        p->tf->regs[10] = sys_write(p->tf->regs[10], (void *)p->tf->regs[11], p->tf->regs[12]);
        break;
    }
    case SYS_read: {
        DEBUG_PRINT(uart_puts("SYS_read\n"););
        p->tf->regs[10] = sys_read(p->tf->regs[10], (void *)p->tf->regs[11], p->tf->regs[12]);
        break;
    }
    case SYS_exit:
        DEBUG_PRINT(uart_puts("SYS_exit\n"););
        p->state = ZOMBIE;
        c->needs_sched = 1;
        break;
    case SYS_getpid:
        DEBUG_PRINT(uart_puts("SYS_getpid\n"););
        p->tf->regs[10] = p->pid;
        break;
    case SYS_yield:
        DEBUG_PRINT(uart_puts("SYS_yield\n"););
        c->needs_sched = 1;
        break;
    case SYS_sleep_ms:
        DEBUG_PRINT(uart_puts("SYS_sleep_ms\n"););
        p->tf->regs[10] = sys_sleep_ms(c, p, p->tf->regs[10]);
        c->needs_sched = 1;
        break;
    case SYS_sbrk:
        p->tf->regs[10] = sys_sbrk(p, p->tf->regs[10]);
        break;
    case SYS_munmap:
        p->tf->regs[10] = sys_munmap(p, p->tf->regs[10], p->tf->regs[11]);
        break;
    default:
        DEBUG_PRINT(
            uart_puts("Unknown syscall number: ");
            uart_puthex(syscall_num);
            uart_putc('\n');
        );
        p->tf->regs[10] = -1;
        break;
    }
}

void
trap_handler(trap_frame_t *tf)
{
    cpu_t *c = curr_cpu();
    proc_t *p = c->proc;

    ulong cause = read_csr(scause);
    ulong code  = SCAUSE_CODE(cause);

    /*
    DEBUG_PRINT(
        uart_puts("Trap: ");
        uart_puthex(code);
        uart_putc('\n');

        uart_puts("sepc: ");
        uart_puthex(read_csr(sepc));
        uart_puts("\n");

        uart_puts("stval: ");
        uart_puthex(read_csr(stval));
        uart_puts("\n");
    );
    */

    // Must not access tf on idle process, it is NULL!
    if (p && p->is_idle) {
        // Timer interrupt on idle core
        if ((cause & SCAUSE_IRQ_BIT) && code == SCAUSE_TIMER_INTERRUPT) {
            timer_handle();
            goto end;
        } else {
            DEBUG_PRINT(
                uart_puts("Trap on idle core? Should not happen, Halting...\n");
            );
            while (1)
                asm volatile("wfi");
        }
    }

    if (SCAUSE_CODE(tf->scause) == 5 && !(tf->scause & SCAUSE_IRQ_BIT)) {
        DEBUG_PRINT(
            uart_puts("Trap: ");
            uart_puthex(SCAUSE_CODE(tf->scause));
            uart_puts("\n");
        );
        while (1)
            ;
    }

    // This shouldn't happen: every CPU should have a proc (idle or otherwise)
    if (!p) {
        // Timer interrupt
        if ((cause & SCAUSE_IRQ_BIT) && code == SCAUSE_TIMER_INTERRUPT) {
            timer_handle();
            return;
        }

        // Weird - should not happen
        DEBUG_PRINT(
            uart_puts("No current process for CPU ");
            uart_putc('0' + c->id);
            uart_puts(" during trap!\n");
        );
        while (1)
            asm volatile("wfi");
    }

    // Capture original process
    //proc_t *orig_proc = p;

    /*
    DEBUG_PRINT(
        uart_puts("Before saving tf: p->tf.sepc = ");
        uart_puthex(p->tf.sepc);
        uart_putc('\n');
    );
    */

    //p->tf = tf;

    // Enable interrupts (allow preemption)
    //set_csr(sstatus, SSTATUS_SIE);

    // Debug
    /*
    DEBUG_PRINT(
        uart_puts("Trap frame at: ");
        uart_puthex((ulong)&p->tf);
        uart_putc('\n');
        uart_puts("scause: ");
        uart_puthex(p->tf.scause);
        uart_putc('\n');
    );
    */

    switch (code) {
    case SCAUSE_USER_ECALL:
        // User-mode (U-mode)
        DEBUG_PRINT(
            uart_puts("[S] U-mode system call received\n");
        );
        tf->sepc += 4;
        syscall_handler(p);
        break;
    case SCAUSE_SUPERVISOR_ECALL:
        // Supervisor-mode (S-mode)
        DEBUG_PRINT(
            uart_puts("[S] S-mode system call received\n");
            //uart_puts("TF a7 = ");
            //uart_puthex(p->tf.regs[17]);  // a7
            //uart_putc('\n');
        );
        tf->sepc += 4;
        syscall_handler(p);
        break;
    case SCAUSE_TIMER_INTERRUPT:
        // Timer interrupt
        if (cause & SCAUSE_IRQ_BIT) {
            /*
            DEBUG_PRINT(
                uart_putc('0' + c->id);
                uart_puts("Normal timer interrupt\n");
            );
            */

            timer_handle();
        }
        break;
    case SCAUSE_SUPERVISOR_IRQ:
        DEBUG_PRINT(
            uart_putc('0' + c->id);
            uart_puts(": [S] Supervisor Software Interrupt\n");
        );
        while(1);
        break;
    case SCAUSE_ILLEGAL_INSTR:
        DEBUG_PRINT(
            uart_puts("CPU ");
            uart_putc('0' + c->id);
            uart_puts(": [S] Illegal instruction at ");
            uart_puthex(tf->sepc);
            uart_putc('\n');
        );
        while(1);
        break;
    default:
        DEBUG_PRINT(
            uart_puts("[S] Unknown trap! cause=");
            uart_puthex(cause);
            uart_puts("\nHalting.\n");
        );
        while (1)
            asm volatile("wfi");
    }

    // Disable interrupts
    //clear_csr(sstatus, SSTATUS_SIE);

    /*
    DEBUG_PRINT(
        uart_puts("Before restoring tf: p->tf->sepc = ");
        uart_puthex(p->tf.sepc);
        uart_putc('\n');
    );
    */

end:
    if (curr_cpu()->needs_sched) {
        curr_cpu()->needs_sched = 0;
        schedule();
    }
}
