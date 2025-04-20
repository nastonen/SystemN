#include "../riscv.h"
#include "../uart.h"
#include "../types.h"
#include "../proc.h"
#include "../timer.h"
#include "trap.h"

#define SCAUSE_IRQ_BIT      (1UL << 63)
#define SCAUSE_CODE(scause) ((scause) & ~SCAUSE_IRQ_BIT)

void
s_trap_handler()
{
    ulong cause = read_csr(scause);
    ulong epc   = read_csr(sepc);
    ulong code  = SCAUSE_CODE(cause);

    // 8 - U-mode, 9 - S-mode
    if (code == 8 || code == 9) {
        spin_lock(&uart_lock);
        uart_puts("[S] System call received\n");
        write_csr(sepc, epc + 4); // skip 'ecall'
        spin_unlock(&uart_lock);
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
