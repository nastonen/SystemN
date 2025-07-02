#include "proc.h"
#include "uart.h"
#include "mm/snub.h"
#include "list.h"
#include "mm/pagetable.h"
#include "trap/trap.h"

static long next_pid;

cpu_t cpus[NCPU];
proc_t idle_procs[NCPU];
extern char _kernel_end[];

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
dump_pagetable_recursive(pte_t *pagetable, int level, ulong va_base)
{
    for (int i = 0; i < 512; i++) {
        if (pagetable[i] & PTE_V) {
            ulong va = va_base | ((ulong)i << (12 + 9 * level));
            ulong pte = pagetable[i];

            uart_puts("L");
            uart_putc('0' + level);
            uart_puts(" Entry ");
            uart_puthex(i);
            uart_puts(": VA ");
            uart_puthex(va);
            uart_puts(" -> PTE ");
            uart_puthex(pte);

            if (pte & (PTE_R | PTE_W | PTE_X)) {
                // It's a leaf entry, calculate physical address
                ulong pa = ((pte >> 10) << 12);
                uart_puts(" -> PA ");
                uart_puthex(pa);
            }

            uart_putc('\n');

            if ((pte & (PTE_R | PTE_W | PTE_X)) == 0) {
                // This PTE points to a lower-level page table
                ulong next_pa = ((pte >> 10) << 12);
                pte_t *next = (pte_t *)PHYS_TO_VIRT(next_pa);
                dump_pagetable_recursive(next, level - 1, va);
            }
        }
    }
}

void
copy_kernel_mappings(pte_t *dst, pte_t *src)
{
    for (ulong va = KERNEL_START_VA; va < (ulong)_kernel_end; va += PAGE_SIZE) {
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
    //DEBUG_PRINT(uart_puts("create_proc()\n"););

    proc_t *p = (proc_t *)kzalloc(sizeof(proc_t));
    if (!p)
        goto fail;

    p->pid = ATOMIC_INC_AND_FETCH(&next_pid);
    p->bound_cpu = -1;

    /*
    DEBUG_PRINT(
        uart_puts("Proc allocated\n");
        uart_puts("Allocating proc pagetable...\n");
    );
    */

    // Allocate user page table
    p->pagetable = alloc_pagetable();
    if (!p->pagetable)
        goto fail1;

    /*
    DEBUG_PRINT(
        uart_puts("Proc pagetable allocated\n");
        uart_puts("Allocating kstack for the proc\n");
    );
    */

    p->kstack = (char *)kzalloc(KSTACK_SIZE);
    if (!p->kstack)
        goto fail1;

    // Map proc_t
    if (map_page(p->pagetable, (ulong)p, VIRT_TO_PHYS(p), PTE_R | PTE_W) == -1)
        goto fail1;

    // Map kstack
    if (map_page(p->pagetable, (ulong)p->kstack, VIRT_TO_PHYS(p->kstack), PTE_R | PTE_W) == -1)
        goto fail1;

    p->tf = (trap_frame_t *)(p->kstack + KSTACK_SIZE - sizeof(trap_frame_t));
    p->tf->regs[2] = USER_STACK_TOP;
    p->tf->sepc = USER_START_VA;
    p->tf->sstatus = SSTATUS_SPIE | SSTATUS_SUM;
    p->ctx.sp = (ulong)p->tf;

    /*
    DEBUG_PRINT(
        uart_puts("Kernel stack allocated and mapped\n");
        uart_puts("Loading user binary...\n");
    );
    */

    // Load user binary
    ulong va = USER_START_VA;
    ulong remaining = binary_size;
    uchar *src = (uchar *)binary;

    while (remaining > 0) {
        ulong user_bin_pa = (ulong)alloc_page();
        if (!user_bin_pa)
            goto fail1;

        ulong to_copy = (remaining > PAGE_SIZE) ? PAGE_SIZE : remaining;
        memcpy(PHYS_TO_VIRT(user_bin_pa), src, to_copy);

        if (map_page(p->pagetable, va, user_bin_pa, PTE_R | PTE_X | PTE_U) == -1)
            goto fail1;

        va += PAGE_SIZE;
        src += to_copy;
        remaining -= to_copy;
    }

    /*
    DEBUG_PRINT(
        uart_puts("User binary loaded\n");
        uart_puts("Allocating user stack...\n");
    );
    */

    // User stack (1 page)
    p->ustack = alloc_page();
    if (!p->ustack)
        goto fail1;

    // Map user stack
    if (map_page(p->pagetable, USER_STACK_TOP - USER_STACK_SIZE, (ulong)p->ustack, PTE_R | PTE_W | PTE_U) == -1)
        goto fail1;

    /*
    DEBUG_PRINT(
        uart_puts("User stack allocated and mapped\n");
        uart_puts("Mapping kernel pagetable into user pagetable\n");
    );
    */

    // Map kernel segments
    copy_kernel_mappings(p->pagetable, PHYS_TO_VIRT(kernel_pagetable));

    /*
    DEBUG_PRINT(
        uart_puts("Kernel pagetable into user pagetable mapped\n");
        uart_puts("Mapping UART to user pagetable\n");
    );
    */

    // Map UART
    if (map_page(p->pagetable, UART0, VIRT_TO_PHYS(UART0), PTE_R | PTE_W) == -1) {
        uart_puts("map_page() failed for UART, halting...\n");
        while (1)
            asm volatile("wfi");
    }

    /*
    DEBUG_PRINT(
        uart_puts("UART mapped into user pagetable\n");
    );
    */

    //dump_mappings(p->pagetable, USER_STACK_TOP - USER_STACK_SIZE, USER_STACK_TOP);
    //dump_mappings(p->pagetable, KERNEL_START_VA, (ulong)_kernel_end);
    //dump_mappings(p->pagetable, UART0, UART0 + PAGE_SIZE);

    // Put into RUNNABLE queue
    p->state = RUNNABLE;
    list_add_tail(&p->q_node, &cpus[curr_cpu()->id].run_queue);

    //dump_pagetable(p->pagetable);
    //uart_puts("User Page Table Dump:\n");
    //dump_pagetable_recursive(p->pagetable, 2, KERNEL_START_VA);
    //dump_pagetable_range(p->pagetable, USER_START_VA, USER_END_VA);
    //uart_puts("And kernel range:\n");
    //dump_pagetable_range(p->pagetable, KERNEL_START_VA, (ulong)_kernel_end);

    return p;

fail1:
    free_proc(p);
fail:
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

    // Free user stack
    if (p->ustack)
        free_page(p->ustack);

    // TODO: properly free user binary

    // Free user page table
    if (p->pagetable)
        free_pagetable(p->pagetable);

    // Free kernel stack
    if (p->kstack)
        kfree(p->kstack);

    // Free the process
    kfree(p);
}
