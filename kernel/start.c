#include "uart.h"
#include "spinlock.h"
#include "riscv.h"
#include "trap/trap.h"
#include "types.h"
#include "proc.h"
#include "timer.h"
#include "syscall.h"
#include "string.h"

void
setup_idle_proc()
{
    struct cpu *c = curr_cpu();
    struct proc *idle = &idle_procs[c->id];
    idle->pid = 0;
    idle->state = RUNNING;
    idle->is_idle = 1;
    c->proc = idle;

    // Create a dummy trap frame to keep things clean
    memset(&idle->tf, 0, sizeof(idle->tf));
    idle->tf.sepc = (ulong)idle_loop;
}

void
test_syscall()
{
    const char *msg = "Hello from write syscall!\n";
    syscall3(SYS_write, 1, (long)msg, strlen(msg));
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

    // Create idle process for each CPU
    setup_idle_proc();

    // Test trap call
    if (curr_cpu()->id == 0) {
        static struct proc test_proc;
        test_proc.pid = 1;
        test_proc.state = RUNNING;
        curr_cpu()->proc = &test_proc;

        test_syscall();
    }

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
    write_csr(pmpaddr0, -1L);   // Top of memory range (all memory)
    write_csr(pmpcfg0, 0x0f);   // R/W/X permissions, TOR mode

    // Set S-mode trap vector
    write_csr(stvec, s_trap_vector);

    // Set up mstatus to enter S-mode
    clear_csr(mstatus, MSTATUS_MPP_MASK);  // Clear MPP (bits 12-11)
    set_csr(mstatus, MSTATUS_MPP_S_MODE);  // Set MPP = S-mode (01)

    // Enable interrupts in S-mode
    set_csr(sstatus, SSTATUS_SIE);

    // Init timer interrupts
    set_csr(sie, SIE_STIE);
    set_csr(menvcfg, MENVCFG_FDT);
    set_csr(mcounteren, MCOUNTEREN_TIME);
    timer_init();

    // Optional: disable paging for now
    write_csr(satp, 0);

    // Set mepc to the address of S-mode entry point
    write_csr(mepc, (ulong)s_mode_main);

    // Drop into S-mode!
    //asm volatile("jalr x0, %0" : : "r"(s_mode_main));  // Jump directly to s_mode_main
    asm volatile("mret");
}
