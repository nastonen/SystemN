#include "trap.h"
#include "../riscv.h"
#include "../uart.h"
#include "../types.h"
#include "../proc.h"
#include "../timer.h"
#include "../syscall.h"
#include "../string.h"

void
syscall_handler(struct proc *p)
{
    trap_frame_t *tf = &p->tf;
    int syscall_num = tf->regs[17]; // a7 (syscall number)

    switch (syscall_num) {
    case SYS_write:
        // 'write' syscall: arguments in a0 (fd), a1 (buffer), a2 (size)
        int fd = tf->regs[10];                  // a0
        const char *buf = (char*)tf->regs[11];  // a1
        ulong len = tf->regs[12];               // a2

        if (fd != 1 && fd != 2) {
            tf->regs[10] = -1;   // error (unsupported fd)
            break;
        }

        spin_lock(&uart_lock);
        uart_puts(buf);
        spin_unlock(&uart_lock);
        tf->regs[10] = len; // return number of bytes written
        break;
    case SYS_exit:
        spin_lock(&uart_lock);
        uart_puts("Exiting program...\n");
        spin_unlock(&uart_lock);
        break;
    case SYS_getpid:
        tf->regs[10] = p->pid; //curr_cpu()->id; // Return hart_id as PID for now
        break;
    default:
        spin_lock(&uart_lock);
        uart_puts("Unknown syscall number: \n");
        uart_puthex(syscall_num);
        uart_putc('\n');
        spin_unlock(&uart_lock);
        tf->regs[10] = -1; // Return error code
        break;
    }
}

void
s_trap_handler(trap_frame_t *tf)
{
    ulong cause = tf->scause;
    ulong code  = SCAUSE_CODE(cause);

    struct cpu *c = curr_cpu();
    struct proc *p = c->proc;

    // Timer interrupt on idle core
    if (p && p->is_idle && (cause & SCAUSE_IRQ_BIT) && code == SCAUSE_TIMER_INTERRUPT) {
        timer_handle();
        return;
    }

    // This shouldn't happen: every CPU should have a proc (idle or otherwise)
    if (!p) {
        // Timer interrupt
        if ((cause & SCAUSE_IRQ_BIT) && code == SCAUSE_TIMER_INTERRUPT) {
            timer_handle();
            return;
        }

        // Weird - should not happen
        spin_lock(&uart_lock);
        uart_puts("No current process for CPU ");
        uart_putc('0' + c->id);
        uart_puts(" during trap!\n");
        spin_unlock(&uart_lock);
        while (1)
            asm volatile("wfi");
    }

    // Capture original process
    //struct proc *orig_proc = p;

    /*
    spin_lock(&uart_lock);
    uart_puts("Before saving tf: p->tf.sepc = ");
    uart_puthex(p->tf.sepc);
    uart_putc('\n');
    spin_unlock(&uart_lock);
    */

    // Save incoming trap frame into the process
    memcpy(&p->tf, tf, sizeof(*tf));

    // Enable interrupts (allow preemption)
    //set_csr(sstatus, SSTATUS_SIE);

    // Debug
    /*
    spin_lock(&uart_lock);
    uart_puts("Trap frame at: ");
    uart_puthex((ulong)&p->tf);
    uart_putc('\n');
    uart_puts("scause: ");
    uart_puthex(p->tf.scause);
    uart_putc('\n');
    spin_unlock(&uart_lock);
    */

    switch (code) {
    case SCAUSE_USER_ECALL:
        // User-mode (U-mode)
        spin_lock(&uart_lock);
        uart_puts("[S] U-mode system call received\n");
        spin_unlock(&uart_lock);
        //write_csr(sepc, epc + 4); // skip 'ecall'
        p->tf.sepc += 4;
        syscall_handler(p);
        break;
    case SCAUSE_SUPERVISOR_ECALL:
        // Supervisor-mode (S-mode)
        spin_lock(&uart_lock);
        uart_puts("[S] S-mode system call received\n");
        //uart_puts("TF a7 = ");
        //uart_puthex(p->tf.regs[17]);  // a7
        //uart_putc('\n');
        spin_unlock(&uart_lock);
        //write_csr(sepc, epc + 4); // skip 'ecall'
        p->tf.sepc += 4;
        syscall_handler(p);
        break;
    case SCAUSE_TIMER_INTERRUPT:
        // Timer interrupt
        if (cause & SCAUSE_IRQ_BIT) {
            timer_handle();
            /*
            spin_lock(&uart_lock);
            uart_putc('0' + c->id);
            uart_puts(": [S] Timer interrupt received\n");
            spin_unlock(&uart_lock);
            */
        }
        break;
    case SCAUSE_SUPERVISOR_IRQ: // Supervisor Software Interrupt
        spin_lock(&uart_lock);
        uart_putc('0' + c->id);
        uart_puts(": [S] Supervisor Software Interrupt\n");
        spin_unlock(&uart_lock);

        // Clear SSIP manually to avoid re-entering
        //clear_csr(sip, (1 << 1));
        break;
    case SCAUSE_ILLEGAL_INSTR:
        spin_lock(&uart_lock);
        uart_puts("CPU ");
        uart_putc('0' + c->id);
        uart_puts(": [S] Illegal instruction at ");
        uart_puthex(tf->sepc);
        uart_putc('\n');
        spin_unlock(&uart_lock);
        break;
    default:
        spin_lock(&uart_lock);
        uart_puts("[S] Unknown trap! cause=");
        uart_puthex(cause);
        uart_puts("\nHalting.\n");
        spin_unlock(&uart_lock);
        while (1)
            asm volatile("wfi");
    }

    // Disable interrupts
    //clear_csr(sstatus, SSTATUS_SIE);

    /*
    spin_lock(&uart_lock);
    uart_puts("Before restoring tf: tf->sepc = ");
    uart_puthex(p->tf.sepc);
    uart_putc('\n');
    spin_unlock(&uart_lock);
    */

    // Restore trap frame
    memcpy(tf, &p->tf, sizeof(*tf));
}
