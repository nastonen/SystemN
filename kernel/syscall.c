#include "syscall.h"
#include "uart.h"
#include "timer.h"
#include "mm/mem.h"
#include "mm/pagetable.h"
#include "mm/user_copy.h"

long
sys_write(int fd, void *user_buf, uint len)
{
    if (fd != 1 && fd != 2)
        return -1;

    if (len > MAXLEN)
        len = MAXLEN;

    char buf[MAXLEN];
    if (copy_from_user(buf, user_buf, len) < 0)
        return -1;

    int written = 0;
    spin_lock(&uart_lock);
    written = uart_putsn(buf, len);
    spin_unlock(&uart_lock);

    // return number of bytes written
    return written;
}

long
sys_read(int fd, void *user_buf, uint len)
{
    if (fd)
        return -1;

    if (len > MAXLEN)
        len = MAXLEN;

    DEBUG_PRINT(uart_puts("waiting for uart_gets()...\n"););

    char buf[MAXLEN];
    int read = uart_gets(buf, len);
    if (copy_to_user(user_buf, buf, read) < 0)
        return -1;

    return read;
}

long
sys_sleep_ms(cpu_t *c, proc_t *p, ulong ms)
{
    if (ms == 0 || ms > MAX_SLEEP_MS)
        return -1;

    /*
    DEBUG_PRINT(
        uart_puts("Proc id ");
        uart_putc('0' + p->pid);
        uart_puts(" going to sleep for ");
        uart_putlong(ms / 1000);
        uart_puts(" seconds\n");
    );
    */

    p->state = SLEEPING;
    list_add_tail(&p->q_node, &c->sleep_queue);
    p->sleep_until = read_time() + MS_TO_TIME(ms);

    return 0;
}

long
sys_sbrk(proc_t *p, long size)
{
    if (!p || !size)
        return -1L;

    ulong old_brk = ALIGN_UP(p->heap_end);
    size = ALIGN_UP(size);

    ulong new_brk = old_brk + size;
    if (new_brk < p->heap_start || new_brk > USER_HEAP_END)
        return -1L;

    // Map new pages
    if (size > 0) {
        for (ulong va = old_brk; va < new_brk; va += PAGE_SIZE) {
            void *pa = alloc_page();
            if (!pa)
                return -1L;

            if (map_page(p->pagetable, va, (ulong)pa, PTE_R | PTE_W | PTE_U) == -1)
                return -1L;
        }
    } else {
        for (ulong va = new_brk; va < old_brk; va += PAGE_SIZE)
            unmap_page(p->pagetable, va);
    }

    p->heap_end = new_brk;

    return old_brk;
}

long
sys_munmap(proc_t *p, ulong addr, ulong size)
{
    if (!p || !size || addr % PAGE_SIZE != 0)
        return -1;

    ulong end = ALIGN_UP(addr + size);
    for (ulong va = addr; va < end; va += PAGE_SIZE)
        if (unmap_page(p->pagetable, va) == -1)
            return -1;

    return 0;
}
