#include "trap.h"
#include "../riscv.h"
#include "../uart.h"
#include "../types.h"
#include "../proc.h"
#include "../timer.h"
#include "../syscall.h"
#include "../string.h"
#include "../sched.h"

void
syscall_handler(proc_t *p)
{
    cpu_t *c = curr_cpu();
    trap_frame_t *tf = p->tf;
    int syscall_num = tf->regs[17]; // a7 (syscall number)
    int fd;
    ulong len;

    switch (syscall_num) {
    case SYS_write:
        // 'write' syscall: arguments in a0 (fd), a1 (buffer), a2 (size)
        DEBUG_PRINT(
            uart_puts("SYS_write\n");
        );

        fd = tf->regs[10];                      // a0
        const char *buf = (char*)tf->regs[11];  // a1
        len = tf->regs[12];                     // a2

        if (fd != 1 && fd != 2) {
            tf->regs[10] = -1;   // error (unsupported fd)
            break;
        }

        spin_lock(&uart_lock);
        uart_puts(buf);
        spin_unlock(&uart_lock);

        tf->regs[10] = len; // return number of bytes written
        break;
    case SYS_read:
        DEBUG_PRINT(
            uart_puts("SYS_read\n");
        );

        fd = tf->regs[10];                      // a0
        char *buf1 = (char *)tf->regs[11];      // a1
        len = tf->regs[12];                     // a2

        if (fd == 0) {// stdin
            DEBUG_PRINT(
                uart_puts("waiting for uart_gets()...\n");
            );
            tf->regs[10] = uart_gets(buf1, len); // return number of bytes read
        } else {
            tf->regs[10] = -1; // invalid
        }
        break;
    case SYS_exit:
        DEBUG_PRINT(
            uart_puts("SYS_exit\n");
        );
        p->state = ZOMBIE;
        c->needs_sched = 1;
        break;
    case SYS_getpid:
        DEBUG_PRINT(
            uart_puts("SYS_getpid\n");
        );
        tf->regs[10] = p->pid;
        break;
    case SYS_yield:
        DEBUG_PRINT(
            uart_puts("SYS_yield\n");
        );

        c->needs_sched = 1;
        break;
    case SYS_sleep_ms:
        DEBUG_PRINT(
            uart_puts("SYS_sleep_ms\n");
        );

        ulong ms = tf->regs[10]; // a0
        if (ms == 0 || ms > MAX_SLEEP_MS) {
            tf->regs[10] = -1; // Return error
            break;
        }
        /*
        DEBUG_PRINT(
            uart_puts("Proc id ");
            uart_putc('0' + p->pid);
            uart_puts(" going to sleep for ");
            uart_putlong(ms / 1000);
            uart_puts(" seconds\n");
        );
        */

        p->state = SLEEPING;
        list_add_tail(&p->q_node, &c->sleep_queue);
        p->sleep_until = read_time() + MS_TO_TIME(ms);
        tf->regs[10] = 0;

        c->needs_sched = 1;
        break;
    default:
        DEBUG_PRINT(
            uart_puts("Unknown syscall number: ");
            uart_puthex(syscall_num);
            uart_putc('\n');
        );
        tf->regs[10] = -1; // Return error code
        break;
    }
}

void
s_trap_handler(trap_frame_t *tf)
{
    cpu_t *c = curr_cpu();
    proc_t *p = c->proc;

    ulong cause = read_csr(scause);
    ulong code  = SCAUSE_CODE(cause);

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

    // Must not access tf on idle process, it is NULL!
    if (p && p->is_idle) {
        // Timer interrupt on idle core
        if ((cause & SCAUSE_IRQ_BIT) && code == SCAUSE_TIMER_INTERRUPT) {
            timer_handle();
            goto end;
            //return;
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

    p->tf = tf;

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
        p->tf->sepc += 4;
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
        p->tf->sepc += 4;
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
    case SCAUSE_SUPERVISOR_IRQ: // Supervisor Software Interrupt
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
            uart_puthex(p->tf->sepc);
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

void
restore_and_sret(trap_frame_t *tf)
{
    /*
    DEBUG_PRINT(
        uart_puts(">> restore_and_sret()\n");
    );
    */

    write_csr(sstatus, tf->sstatus);
    write_csr(sepc, tf->sepc);

    /*
    DEBUG_PRINT(
        uart_puts(">> About to sret to PC = ");
        uart_puthex(tf->sepc);
        uart_puts("\n>> sp = ");
        uart_puthex(tf->regs[2]);
        uart_putc('\n');
    );
    */

    asm volatile(
        // Restore registers from tf->regs[]
        "ld ra, 8*1(%0)\n"
        "ld sp, 8*2(%0)\n"
        "ld gp, 8*3(%0)\n"
        "ld tp, 8*4(%0)\n"
        "ld t0, 8*5(%0)\n"
        "ld t1, 8*6(%0)\n"
        "ld t2, 8*7(%0)\n"
        "ld s0, 8*8(%0)\n"
        "ld s1, 8*9(%0)\n"
        "ld a0, 8*10(%0)\n"
        "ld a1, 8*11(%0)\n"
        "ld a2, 8*12(%0)\n"
        "ld a3, 8*13(%0)\n"
        "ld a4, 8*14(%0)\n"

        "ld a5, 8*15(%0)\n"

        "ld a6, 8*16(%0)\n"
        "ld a7, 8*17(%0)\n"
        "ld s2, 8*18(%0)\n"
        "ld s3, 8*19(%0)\n"
        "ld s4, 8*20(%0)\n"
        "ld s5, 8*21(%0)\n"
        "ld s6, 8*22(%0)\n"
        "ld s7, 8*23(%0)\n"
        "ld s8, 8*24(%0)\n"
        "ld s9, 8*25(%0)\n"
        "ld s10, 8*26(%0)\n"
        "ld s11, 8*27(%0)\n"
        "ld t3, 8*28(%0)\n"
        "ld t4, 8*29(%0)\n"
        "ld t5, 8*30(%0)\n"
        "ld t6, 8*31(%0)\n"
        "sret\n"
        :
        : "r"(tf->regs)
        : "memory"
    );
}
