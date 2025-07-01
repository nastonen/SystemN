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
#include "mm/pagetable.h"

volatile int allocator_ready = 0;

extern char _stack[];

extern char _binary_shell_bin_start[];
extern char _binary_shell_bin_end[];

extern char _kernel_text_start[];
extern char _kernel_text_end[];
extern char _kernel_rodata_start[];
extern char _kernel_rodata_end[];
extern char _kernel_data_start[];

void
shell_init()
{
    ulong size = _binary_shell_bin_end - _binary_shell_bin_start;
    proc_t *shell = create_proc(_binary_shell_bin_start, size);

    list_del(&shell->q_node);
    shell->state = RUNNING;
    shell->bound_cpu = 0;

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
    idle->pagetable = kernel_pagetable;

    // Reuse boot stack as idle stack
    idle->kstack = _stack + (c->id * KSTACK_SIZE);

    // Set context
    idle->ctx.ra = (ulong)idle_loop;
    idle->ctx.sp = (ulong)(idle->kstack + KSTACK_SIZE);

    if (c->id)
        c->proc = idle;
}

void jump_to_user_shell()
{
    DEBUG_PRINT(uart_puts("Jumping to user shell\n"););

    proc_t *p = curr_cpu()->proc;

    // Setup SSTATUS: SPIE=1 (enable interrupts)
    write_csr(sstatus, SSTATUS_SPIE);
    // Set return mode = user
    clear_csr(sstatus, SSTATUS_SPP);
    // Save trap frame
    write_csr(sscratch, (ulong)p->tf);

    load_pagetable(p->pagetable);

    // Set up sp (toggle SUM to touch user memory)
    set_csr(sstatus, SSTATUS_SUM);
    register ulong sp asm("sp") = (ulong)USER_STACK_TOP;
    clear_csr(sstatus, SSTATUS_SUM);

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

    // Create per-CPU process queues
    LIST_HEAD_INIT(&cpus[curr_cpu()->id].run_queue);
    LIST_HEAD_INIT(&cpus[curr_cpu()->id].sleep_queue);

    // Enable timer irqs only after queues are set up
    timer_init();

    // Create idle process for each CPU
    setup_idle_proc();

    // Jump to shell (future init) on core0
    if (curr_cpu()->id == 0) {
        shell_init();
        jump_to_user_shell();
    } else {
        idle_loop();
    }

    DEBUG_PRINT(uart_puts("End of s_mode_main()\n"););
    while (1)
        asm volatile("wfi");
}

void
setup_kernel_pagetable()
{
    /*
    DEBUG_PRINT(
        uart_puts("calling map_page() on KERNEL_START - KERNEL_END\n");
    );
    */

    // Map kernel code
    for (ulong pa = (ulong)_kernel_text_start; pa < (ulong)_kernel_text_end; pa += PAGE_SIZE) {
        if (map_page(kernel_pagetable, pa + VA_OFFSET, pa, PTE_R | PTE_X) == -1) {
            uart_puts("map_page() failed for kernel text, halting...\n");
            while (1)
                asm volatile("wfi");
        }
    }

    // Map rodata
    for (ulong pa = (ulong)_kernel_rodata_start; pa < (ulong)_kernel_rodata_end; pa += PAGE_SIZE) {
        if (map_page(kernel_pagetable, pa + VA_OFFSET, pa, PTE_R) == -1) {
            uart_puts("map_page() failed for kernel rodata, halting...\n");
            while (1)
                asm volatile("wfi");
        }
    }

    // Map (s)data, (s)bss and heap
    for (ulong pa = (ulong)_kernel_data_start; pa < (ulong)KERNEL_END; pa += PAGE_SIZE) {
        if (map_page(kernel_pagetable, pa + VA_OFFSET, pa, PTE_R | PTE_W) == -1) {
            uart_puts("map_page() failed for kernel data + heap, halting...\n");
            while (1)
                asm volatile("wfi");
        }
    }

    // Map UART
    if (map_page(kernel_pagetable, UART0, UART0 - VA_OFFSET, PTE_R | PTE_W) == -1) {
        uart_puts("map_page() failed for UART, halting...\n");
        while (1)
            asm volatile("wfi");
    }

    // Wait for the changes
    asm volatile ("sfence.vma zero, zero");
}

void
mm_init()
{
    // Initialize global memory allocator
    buddy_allocator_init();

    // Map phys mem to virt in kernel
    setup_kernel_pagetable();

    // Use kernel page table
    load_pagetable(kernel_pagetable);

    // SystemN Unified Buddy allocator :)
    snub_init();

    // Adjust addrs to virtual for S-mode
    va_offset = VA_OFFSET;
}

void
start()
{
    // Set struct cpu to tp register
    uint hart_id = read_csr(mhartid);
    write_tp((ulong)&cpus[hart_id]);

    // Set hart_id to struct cpu
    curr_cpu()->id = hart_id;

    // Halt all harts except 0
    //if (hart_id != 0)
      //  while (1)
        //    asm volatile("wfi");

    // Initialize memory system
    if (hart_id == 0) {
        mm_init();
        __sync_synchronize();
        allocator_ready = 1;
    } else {
        // Wait for hart 0 to finish memory init
        while (!allocator_ready)
            asm volatile("nop");

        // Use kernel page table on all cores
        __sync_synchronize();
        load_pagetable(kernel_pagetable);
    }

    // Delegate exceptions and interrupts to S-mode
    write_csr(medeleg, 0xffff);
    write_csr(mideleg, 0xffff);

    // Setup PMP to give S-mode access to all memory
    write_csr(pmpaddr0, -1L);
    write_csr(pmpcfg0, 0x0f);   // R/W/X permissions, TOR mode

    // Set S-mode trap vector
    write_csr(stvec, trap_vector + VA_OFFSET);

    // Set up mstatus to enter S-mode
    clear_csr(mstatus, MSTATUS_MPP_MASK);  // Clear MPP (bits 12-11)
    set_csr(mstatus, MSTATUS_MPP_S_MODE);  // Set MPP = S-mode (01)

    // Enable interrupts in S-mode
    set_csr(sstatus, SSTATUS_SIE);

    // Supervisor External Interrupt Enable
    //set_csr(sie, SIE_SEIE);

    // Init timer interrupts
    set_csr(sie, SIE_STIE);
    set_csr(menvcfg, MENVCFG_FDT);
    set_csr(mcounteren, MCOUNTEREN_TIME);

    // Set mepc to the address of S-mode entry point
    write_csr(mepc, (ulong)s_mode_main + VA_OFFSET);

    // Adjust registers to virtual for S-mode
    write_tp((ulong)&cpus[hart_id] + VA_OFFSET);
    asm volatile("add sp, sp, %0" :: "r"(VA_OFFSET));

    // Drop to S-mode!
    asm volatile("mret");
}
