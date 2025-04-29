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
#include "shell.h"

//char shell_stack[4096]; // 4 KiB user stack
extern char _binary_shell_bin_start[];
extern char _binary_shell_bin_end[];

#define USER_CODE_START 0x80020000
#define USER_STACK_SIZE 4096

//char user_stack[USER_STACK_SIZE];
#define user_stack (USER_CODE_START + 0x1000)

void
proc_trampoline()
{
    spin_lock(&uart_lock);
    uart_puts(">> Entered proc_trampoline()\n");
    spin_unlock(&uart_lock);

    struct proc *p = curr_cpu()->proc;
    restore_and_sret(&p->tf);  // This will drop into user mode at tf.sepc

    spin_lock(&uart_lock);
    uart_puts(">> Return from proc_trampoline()\n");
    spin_unlock(&uart_lock);
}


void
shell_init()
{
    struct proc *p = &proc_table[0];
    memset(p, 0, sizeof(*p));
    p->pid = 1;
    p->bound_cpu = 0; // remember to set to -1 by default
    p->state = RUNNABLE;
    //p->is_idle = 0;

    // Copy user code to USER_CODE_START
    uint user_code_size = _binary_shell_bin_end - _binary_shell_bin_start;
    memcpy((void *)USER_CODE_START, _binary_shell_bin_start, user_code_size);

    spin_lock(&uart_lock);
    uart_puts("Shell code copied to ");
    uart_puthex(USER_CODE_START);
    uart_puts("\nFirst 4 bytes: ");
    uint *p1 = (uint *)USER_CODE_START;
    uart_puthex(p1[0]);
    uart_puts("\n");
    spin_unlock(&uart_lock);

    // Set up trap frame
    memset(&p->tf, 0, sizeof(p->tf));
    p->tf.sepc = USER_CODE_START; //(ulong)user_shell_main;
    p->tf.sstatus = SSTATUS_SPIE; // | SSTATUS_UPIE; // SPP=0 -> user mode
    //p->tf.regs[2] = (ulong)(shell_stack + sizeof(shell_stack));
    p->tf.regs[2] = (ulong)user_stack; // (user_stack + USER_STACK_SIZE);  // User stack pointer

    // Set context
    p->ctx.ra = (ulong)proc_trampoline;
    //p->ctx.sp = (ulong)(shell_stack + sizeof(shell_stack));
    p->ctx.sp = (user_stack + USER_STACK_SIZE); // optional
}

void
setup_idle_proc()
{
    struct cpu *c = curr_cpu();
    struct proc *idle = &idle_procs[c->id];

    idle->pid = 0;
    idle->state = RUNNING;
    idle->is_idle = 1;

    // Create a dummy trap frame to keep things clean
    memset(&idle->tf, 0, sizeof(idle->tf));
    idle->tf.sepc = (ulong)idle_loop;
    idle->tf.sstatus = SSTATUS_SPIE | SSTATUS_SPP;

    // Set context
    idle->ctx.ra = (ulong)idle_loop;
    idle->ctx.sp = (ulong)&idle_stack[c->id][1024];

    c->proc = idle;
}

void jump_to_user_shell()
{
    ulong user_pc = USER_CODE_START; //0x80020000;      // shell binary entry
    ulong user_sp = user_stack; //0x80021000;      // user stack top

    // Setup SSTATUS: SPIE=1 (enable interrupts), SPP=0 (user mode)
    ulong sstatus = SSTATUS_SPIE;
    write_csr(sstatus, sstatus);

    write_csr(sepc, user_pc);        // Set exception return PC

    // Set up user registers (only sp is required here)
    register ulong sp asm("sp") = user_sp;
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
    spin_lock(&uart_lock);
    uart_puts("Hart ");
    uart_putc('0' + curr_cpu()->id);
    uart_puts(": Hello from S-mode!\n");
    spin_unlock(&uart_lock);

    // Create idle process for each CPU
    setup_idle_proc();

    // Test trap call
    if (curr_cpu()->id == 0) {
        shell_init();

        // Manually?
        //struct proc *p = &proc_table[0];
        //restore_and_sret(&p->tf);
    }

    jump_to_user_shell();

    /*
    spin_lock(&uart_lock);
    uart_puts("tf->sstatus: ");
    uart_puthex(curr_cpu()->proc->tf.sstatus);
    uart_puts("\n");
    spin_unlock(&uart_lock);
    schedule();
    */
    // called in trampoline!
    //restore_and_sret(&curr_cpu()->proc->tf);

    while (1) {
        spin_lock(&uart_lock);
        uart_puts("End of s_mode_main()\n");
        spin_unlock(&uart_lock);

        asm volatile("wfi");
    }
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

    // Halt all harts except 0
    if (hart_id != 0) {
        while (1) {
            asm volatile("wfi");
        }
    }

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
