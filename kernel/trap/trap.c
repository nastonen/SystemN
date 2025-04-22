#include "trap.h"
#include "../riscv.h"
#include "../uart.h"
#include "../types.h"
#include "../proc.h"
#include "../timer.h"
#include "../syscall.h"

#define SCAUSE_IRQ_BIT      (1UL << 63)
#define SCAUSE_CODE(scause) ((scause) & ~SCAUSE_IRQ_BIT)

void
syscall_handler(trap_frame_t *tf)
{
    int syscall_num = tf->regs[17]; // a7 (syscall number)
    switch(syscall_num) {
        case SYS_write:
            // 'write' syscall: arguments in a0 (fd), a1 (buffer), a2 (size)
            spin_lock(&uart_lock);
            uart_puts((char*)tf->regs[10]);
            spin_unlock(&uart_lock);
            tf->regs[10] = 0;
            break;
        case SYS_exit:
            spin_lock(&uart_lock);
            uart_puts("Exiting program...\n");
            spin_unlock(&uart_lock);
            while (1)
                asm volatile("wfi"); // Halt the system (for now)
            break;
        case SYS_getpid:
            tf->regs[10] = curr_cpu()->id; // Return hart_id as PID for now
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
    ulong cause = tf->scause; //read_csr(scause);
    //ulong epc   = tf->sepc; //read_csr(sepc);
    ulong code  = SCAUSE_CODE(cause);

    /* Debug
    spin_lock(&uart_lock);
    uart_puts("Trap frame at: ");
    uart_puthex((ulong)tf);
    uart_putc('\n');
    uart_puts("scause: ");
    uart_puthex(tf->scause);
    uart_putc('\n');
    spin_unlock(&uart_lock);
    */

    if (code == 8) {
        // User-mode (U-mode)
        spin_lock(&uart_lock);
        uart_puts("[S] U-mode system call received\n");
        spin_unlock(&uart_lock);
        //write_csr(sepc, epc + 4); // skip 'ecall'
        tf->sepc += 4;
        syscall_handler(tf);
    } else if (code == 9) {
        // Supervisor-mode (S-mode)
        spin_lock(&uart_lock);
        uart_puts("[S] S-mode system call received\n");
        //uart_puts("TF a7 = ");
        //uart_puthex(tf->regs[17]);  // a7
        //uart_putc('\n');
        spin_unlock(&uart_lock);
        //write_csr(sepc, epc + 4); // skip 'ecall'
        tf->sepc += 4;
        syscall_handler(tf);
    } else if ((cause & SCAUSE_IRQ_BIT) && code == 5) {
        // Timer interrupt
        timer_handle();

        /*
        spin_lock(&uart_lock);
        uart_putc('0' + curr_cpu()->id);
        uart_puts(": [S] Timer interrupt received\n");
        spin_unlock(&uart_lock);
        */
    } else {
        spin_lock(&uart_lock);
        uart_puts("[S] Unknown trap! cause=");
        uart_puthex(cause);
        uart_puts("\nHalting.\n");
        spin_unlock(&uart_lock);

        while (1)
            asm volatile("wfi");
    }
}
