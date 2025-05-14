#include "uart.h"
#include "spinlock.h"
#include "riscv.h"
#include "trap/trap.h"
#include "types.h"
#include "proc.h"
#include "timer.h"
#include "syscall.h"
#include "string.h"
#include "sched.h"
#include "mm/snub.h"

#define USER_STACK_SIZE 4096
#define USER_STACK_TOP (USER_START + USER_STACK_SIZE)

volatile int allocator_ready = 0;
extern char _kernel_end[];
extern char _binary_shell_bin_start[];
extern char _binary_shell_bin_end[];

void
shell_init()
{
    proc_t *shell = create_proc();
    list_del(&shell->q_node);
    shell->state = RUNNING;
    shell->bound_cpu = 0;

    // Copy user code to USER_START
    uint user_code_size = _binary_shell_bin_end - _binary_shell_bin_start;
    memcpy((void *)USER_START, _binary_shell_bin_start, user_code_size);

    curr_cpu()->proc = shell;
}

void
setup_idle_proc()
{
    cpu_t *c = curr_cpu();
    proc_t *idle = &idle_procs[c->id];

    memset(idle, 0, sizeof(proc_t));
    //idle->pid = 0;
    idle->is_idle = 1;
    idle->state = RUNNING;

    // Set context
    idle->ctx.ra = (ulong)idle_loop;
    idle->ctx.sp = (ulong)(idle->kstack + KSTACK_SIZE);

    if (c->id)
        c->proc = idle;
}

void jump_to_user_shell()
{
    // Setup SSTATUS: SPIE=1 (enable interrupts)
    write_csr(sstatus, (ulong)SSTATUS_SPIE);
    // Set exception return PC
    write_csr(sepc, (ulong)USER_START);
    // Set up user registers (only sp is required here)
    register ulong sp asm("sp") = (ulong)USER_STACK_TOP;

    asm volatile(
        "mv sp, %0\n"
        "sret\n"
        :
        : "r"(sp)
        : "memory"
    );
}

void
s_mode_main()
{
    // Just print hello for now
    DEBUG_PRINT(
        uart_puts("Hart ");
        uart_putc('0' + curr_cpu()->id);
        uart_puts(": Hello from S-mode!\n");
    );

    // Create idle process for each CPU
    setup_idle_proc();

    // Jump to shell (future init) on core0
    if (curr_cpu()->id == 0) {
        shell_init();
        jump_to_user_shell();
    } else {
        idle_loop();
    }

    DEBUG_PRINT(
        uart_puts("End of s_mode_main()\n");
    );
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
    cpu_t *c = curr_cpu();
    c->id = hart_id;

    if (c->id == 0) {
        // Initialize global kernel memory allocator
        buddy_allocator_init(_kernel_end, (void *)KERNEL_END);
        // SystemN Unified Buddy allocator :)
        snub_init();
        allocator_ready = 1;
    } else {
        // Wait for hart 0 to finish allocator init
        while (!allocator_ready)
            asm volatile("nop");
    }

    // Create per-CPU process queues
    LIST_HEAD_INIT(&cpus[c->id].run_queue);
    LIST_HEAD_INIT(&cpus[c->id].sleep_queue);

    // Halt all harts except 0
    //if (hart_id != 0)
      //  while (1)
        //    asm volatile("wfi");

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
