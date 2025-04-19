#include "uart.h"
#include "spinlock.h"
#include "riscv.h"
#include "trap/trap.h"
#include "types.h"
#include "proc.h"

//static spinlock_t uart_lock = SPINLOCK_INIT;

void s_mode_main();

void
start()
{
    // Set struct cpu to tp register
    uint hart_id = read_csr(mhartid);
    write_tp((ulong)&cpus[hart_id]);

    // Set hart_id to struct cpu
    struct cpu *c = curr_cpu();
    c->id = hart_id;

    // Delegate exceptions and interrupts to S-mode
    write_csr(medeleg, 0xffff);
    write_csr(mideleg, 0xffff);
    //ulong sie = read_csr(sie);
    //write_csr(sie, sie | (1L << 9) | (1L << 5) | (1L << 1));

    // Setup PMP to give S-mode access to all memory
    write_csr(pmpaddr0, -1L);     // Top of memory range (all memory)
    write_csr(pmpcfg0, 0x0f);     // R/W/X permissions, TOR mode

    // Set S-mode trap vector
    write_csr(stvec, s_trap_vector);

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
    //asm volatile("jalr x0, %0" : : "r"(s_mode_main));  // Jump directly to s_mode_main
    asm volatile("mret");
}

void
s_mode_main()
{
    // Enable interrupts in S-mode
    ulong sstatus = read_csr(sstatus);
    sstatus |= (1UL << 1); // SIE bit
    write_csr(sstatus, sstatus);

    // Test trap call
    //asm volatile("ecall");

    spin_lock(&uart_lock);
    uart_puts("Hart ");
    uart_putc('0' + curr_cpu()->id);
    uart_puts(": Hello from S-mode!\n");
    spin_unlock(&uart_lock);

    while (1)
        asm volatile("wfi");
}
