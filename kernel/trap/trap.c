#include "../riscv.h"
#include "../uart.h"
#include "../types.h"
#include "trap.h"

void
s_trap_handler()
{
    ulong cause = read_csr(scause);
    ulong epc = read_csr(sepc);

    // 8 - U-mode, 9 - S-mode
    if ((cause & 0xfff) == 8 || (cause & 0xfff) == 9) {
        spin_lock(&uart_lock);
        uart_puts("[S] System call received\n");
        write_csr(sepc, epc + 4); // skip 'ecall'
        spin_unlock(&uart_lock);
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
