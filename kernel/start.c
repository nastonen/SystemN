#include "uart.h"
#include "spinlock.h"
#include "riscv.h"
#include "trap/trap.h"
#include "types.h"
#include "proc.h"
#include "timer.h"
#include "syscall.h"

void
test_syscall()
{
    syscall3(SYS_write, (long)"Hello from wrapped syscall!\n", 0, 0);
    syscall0(SYS_exit); // Halts the system
}

void
s_mode_main()
{
    // Just print hello for now
    spin_lock(&uart_lock);
    uart_puts("Hart ");
    uart_putc('0' + curr_cpu()->id);
    uart_puts(": Hello from S-mode!\n");
    spin_unlock(&uart_lock);

    // Test trap call
    if (curr_cpu()->id == 0)
        test_syscall();

    while (1)
        asm volatile("wfi");
}

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

    // Enable interrupts in S-mode
    write_csr(sstatus, read_csr(sstatus) | (1L << 1));

    // Init timer interrupts
    write_csr(sie, read_csr(sie) | (1L << 5));
    write_csr(menvcfg, read_csr(menvcfg) | (1L << 63));
    write_csr(mcounteren, read_csr(mcounteren) | (1L << 1));
    timer_init();

    // Optional: disable paging for now
    write_csr(satp, 0);

    // Set mepc to the address of S-mode entry point
    write_csr(mepc, (ulong)s_mode_main);

    // Drop into S-mode!
    //asm volatile("jalr x0, %0" : : "r"(s_mode_main));  // Jump directly to s_mode_main
    asm volatile("mret");
}
