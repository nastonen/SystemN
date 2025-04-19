#include "uart.h"
#include "spinlock.h"
#include "riscv.h"
#include "trap/trap.h"
#include "types.h"
#include "proc.h"

//static spinlock_t uart_lock = SPINLOCK_INIT;

void
start()
{
    // Delegate exceptions and interrupts to S-mode
    write_csr(medeleg, 0xffff);
    write_csr(mideleg, 0xffff);

    // Set S-mode trap vector
    write_csr(stvec, (ulong)s_trap_vector);

    // Set up mstatus to enter S-mode
    ulong mstatus = read_csr(mstatus);
    mstatus &= ~(3UL << 11);      // Clear MPP
    mstatus |= (1UL << 11);       // Set MPP = S-mode (01)
    write_csr(mstatus, mstatus);

    // Set mepc to the address of S-mode entry point
    write_csr(mepc, (ulong)s_mode_main);

    // Optional: disable paging for now
    write_csr(satp, 0);

    // Drop into S-mode!
    asm volatile("mret");


    // Print to UART
    /*
    spin_lock(&uart_lock);

    uart_puts("Hart ");
    uart_putc('0' + read_mhartid());
    uart_puts(": Hello from RISC-V!\n");

    spin_unlock(&uart_lock);
    */
}

void
s_mode_main()
{
    // Set hartid to tp register
    write_tp((ulong)&cpus[read_csr(mhartid)]);

    // Set hartid to struct cpu
    struct cpu *c = curr_cpu();
    c->id = read_csr(mhartid);

    // Enable interrupts in S-mode
    ulong sstatus = read_csr(sstatus);
    sstatus |= (1 << 1); // SIE bit
    write_csr(sstatus, sstatus);

    // For now, just print a message using UART
    volatile char *uart = (char *)0x10000000;
    const char *msg = "Hello from S-mode!\n";

    while (*msg)
        *uart = *msg++;

    while (1)
        asm volatile("wfi");
}
