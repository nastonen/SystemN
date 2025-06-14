#include "proc.h"
#include "uart.h"
#include "mm/snub.h"
#include "list.h"
#include "mm/pagetable.h"
#include "trap/trap.h"

static long next_pid = 1;

cpu_t cpus[NCPU];
proc_t idle_procs[NCPU];
char idle_stack[NCPU][KSTACK_SIZE];

void
idle_loop()
{
    DEBUG_PRINT(
        uart_puts("Hart ");
        uart_putc('0' + curr_cpu()->id);
        uart_puts(": idle loop\n");
    );

    // Set simplified trap vector
    write_csr(stvec, idle_trap_vector);

    // Enable interrupts
    set_csr(sstatus, SSTATUS_SIE);

    while (1)
        asm volatile("wfi");  // Wait for interrupt
}

void
dump_mappings(pte_t *pagetable, ulong start_va, ulong end_va)
{
    for (ulong va = start_va; va < end_va; va += PAGE_SIZE) {
        pte_t *pte = walk(pagetable, va, 0);
        if (!pte || !(*pte & PTE_V)) {
            uart_puts("VA not mapped: ");
            uart_puthex(va);
            uart_putc('\n');
            continue;
        }

        uart_puts("VA: ");
        uart_puthex(va);
        uart_puts(" -> PA: ");
        uart_puthex(PTE2PA(*pte));
        uart_puts(" Flags: ");
        uart_puthex(PTE_FLAGS(*pte));
        uart_putc('\n');
    }
}

void
copy_kernel_mappings(pte_t *dst, pte_t *src)
{
    // Copy all mappings from KERNEL_START to KERNEL_END
    for (ulong va = KERNEL_START; va < KERNEL_END; va += PAGE_SIZE) {
        pte_t *src_pte = walk(src, va, 0);
        if (!src_pte || !(*src_pte & PTE_V)) {
            uart_puts("copy_kernel_mappings: missing PTE at ");
            uart_puthex(va);
            uart_putc('\n');
            continue;
        }

        ulong pa = PTE2PA(*src_pte);
        ulong flags = PTE_FLAGS(*src_pte);

        if (map_page(dst, va, pa, flags) != 0) {
            uart_puts("copy_kernel_mappings: map_page failed at ");
            uart_puthex(va);
            uart_putc('\n');
        }
    }
}

proc_t *
create_proc(void *binary, ulong binary_size)
{
    DEBUG_PRINT(uart_puts("create_proc()\n"););

    proc_t *p = (proc_t *)kzalloc(sizeof(proc_t));
    if (!p)
        goto fail1;

    p->pid = ATOMIC_FETCH_AND_INC(&next_pid);
    p->bound_cpu = -1;

    p->kstack = (char *)kzalloc(KSTACK_SIZE);
    if (!p->kstack)
        goto fail;

    p->tf = (trap_frame_t *)(p->kstack + KSTACK_SIZE - sizeof(trap_frame_t));
    p->tf->regs[2] = USER_STACK_TOP;
    p->tf->sepc = USER_START;
    p->tf->sstatus = SSTATUS_SPIE | SSTATUS_SUM;

    p->ctx.sp = (ulong)p->tf;
    //p->ctx.ra = (ulong)proc_trampoline;

    // Allocate user page table
    p->pagetable = alloc_pagetable();
    if (!p->pagetable)
        goto fail;

    // Load user binary
    ulong va = USER_START;
    ulong remaining = binary_size;
    uchar *src = (uchar *)binary;

    while (remaining > 0) {
        ulong pa = (ulong)alloc_page();
        if (!pa)
            goto fail;

        ulong to_copy = (remaining > PAGE_SIZE) ? PAGE_SIZE : remaining;
        memcpy((void *)pa, src, to_copy);
        if (map_page(p->pagetable, va, pa, PTE_R | PTE_X | PTE_U) == -1)
            goto fail;

        va += PAGE_SIZE;
        src += to_copy;
        remaining -= to_copy;
    }

    // User stack (1 page)
    ulong stack_pa = (ulong)alloc_page();
    if (!stack_pa)
        goto fail;
    if (map_page(p->pagetable, USER_STACK_TOP - PAGE_SIZE, stack_pa, PTE_R | PTE_W | PTE_U) == -1)
        goto fail;

    // Map kernel segments
    copy_kernel_mappings(p->pagetable, kernel_pagetable);
    //for (ulong va = KERNEL_START; va < KERNEL_END; va += PAGE_SIZE)
        //map_page(p->pagetable, va, va, PTE_R | PTE_W | PTE_X);

    // Map UART
    if (map_page(p->pagetable, UART0, UART0, PTE_R | PTE_W) == -1) {
        uart_puts("map_page() failed for UART, halting...\n");
        while (1)
            asm volatile("wfi");
    }

    //dump_mappings(p->pagetable, USER_STACK_TOP - PAGE_SIZE, USER_STACK_TOP);
    //dump_mappings(p->pagetable, KERNEL_START, KERNEL_END);
    //dump_mappings(p->pagetable, UART0, UART0 + PAGE_SIZE);

    // Put into RUNNABLE queue
    p->state = RUNNABLE;
    list_add_tail(&p->q_node, &cpus[curr_cpu()->id].run_queue);

    return p;

fail:
    free_proc(p);
fail1:
    DEBUG_PRINT(uart_puts("Process creation failed. Return NULL.\n"););
    return NULL;
}

void
free_proc(proc_t *p)
{
    if (!p)
        return;

    // If it's in a queue, remove it
    if (list_in_queue(&p->q_node))
        list_del(&p->q_node);

    // Free user page table
    if (p->pagetable)
        free_pagetable(p->pagetable);

    // TODO: properly free user binary and stack

    // Free kernel stack
    if (p->kstack)
        kfree(p->kstack);

    kfree(p);
}
