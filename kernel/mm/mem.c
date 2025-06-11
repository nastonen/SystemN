#include "mem.h"
#include "../spinlock.h"
#include "../uart.h"

extern char _kernel_end[];
static spinlock_t buddy_lock = SPINLOCK_INIT;
static page_t page_array[NUM_PAGES];
static page_t *free_lists[MAX_ORDER + 1];

__attribute__((aligned(PAGE_SIZE)))
pte_t kernel_pagetable[PAGE_ENTRIES];

static inline ulong
page_idx(void *addr)
{
    return ((ulong)addr - BUDDY_BASE_PHYS) / PAGE_SIZE;
}

static inline page_t *
get_page_struct(void *addr)
{
    ulong idx = page_idx(addr);
    if (idx >= NUM_PAGES) {
        spin_lock(&uart_lock);
        uart_puts("get_page_struct: INVALID ADDR ");
        uart_puthex((ulong)addr);
        uart_putc('\n');
        spin_unlock(&uart_lock);
        while (1)
            asm volatile("wfi");
    }
    return &page_array[idx];

    //return &page_array[page_idx(addr)];
}

static inline void *
page_addr(ulong idx)
{
    if (idx >= NUM_PAGES) {
        spin_lock(&uart_lock);
        uart_puts("page_addr: INVALID IDX ");
        uart_putlong(idx);
        uart_putc('\n');
        spin_unlock(&uart_lock);
        while (1)
            asm volatile("wfi");
    }
    return (void *)(BUDDY_BASE_PHYS + idx * PAGE_SIZE);
}

static void *
buddy_of(void *addr, int order)
{
    ulong block = (ulong)addr - BUDDY_BASE_PHYS;
    ulong size = PAGE_SIZE << order;
    ulong buddy = block ^ size;

    return (void *)(buddy + BUDDY_BASE_PHYS);
}

int
buddy_allocator_init()
{
    if ((ulong)_kernel_end >= (ulong)BUDDY_BASE_PHYS)
        return -1;

    ulong curr = (ulong)BUDDY_BASE_PHYS;
    ulong end = (ulong)KERNEL_END;

    for (int i = 0; i <= MAX_ORDER; i++)
        free_lists[i] = NULL;

    while (curr + PAGE_SIZE <= end) {
        int order = MAX_ORDER;

        // If not enough space in this order or not aligned
        // to power-of-two, reduce order
        while (order > 0 &&
               (curr + (PAGE_SIZE << order) > end ||
               curr & ((PAGE_SIZE << order) - 1))) {
            order--;
        }

        page_t *pg = get_page_struct((void *)curr);
        pg->order = order;
        pg->is_free = true;
        pg->next = free_lists[order];
        free_lists[order] = pg;

        curr += PAGE_SIZE << order;
    }

    return 0;
}

void *
alloc_page(void)
{
    return alloc_pages(0);
}

void *
alloc_pages(int order)
{
    if (order > MAX_ORDER)
        return NULL;

    // Lock spinlock
    spin_lock(&buddy_lock);

    for (int o = order; o <= MAX_ORDER; o++) {
        if (!free_lists[o])
            continue;

        // Pop from freelist
        page_t *block = free_lists[o];
        block->order = order;
        block->is_free = false;
        free_lists[o] = block->next;

        // Split higher orders
        int block_idx = block - page_array;

        for (int curr = o; curr > order; curr--) {
            uint buddy_idx = block_idx + (1 << (curr - 1));
            if (buddy_idx >= NUM_PAGES) {
                uart_puts("ERROR: buddy_idx out of bounds\n");
                while (1)
                    asm volatile("wfi");
            }

            page_t *buddy = &page_array[buddy_idx];
            buddy->order = curr - 1;
            buddy->is_free = true;
            buddy->next = free_lists[curr - 1];
            free_lists[curr - 1] = buddy;
        }

        // Set final order
        //block->order = order;

        // Get the address
        void *addr = page_addr(block_idx);

        // Unlock spinlock
        spin_unlock(&buddy_lock);

        // Zero the memory
        memset(addr, 0, PAGE_SIZE << order);

        return addr;
    }

    // Unlock spinlock
    spin_unlock(&buddy_lock);

    //uart_puts("returning null\n");

    return NULL;
}

void
free_page(void *ptr)
{
    free_pages(ptr);
}

void
free_pages(void *addr)
{
    // Lock spinlock
    spin_lock(&buddy_lock);

    page_t *pg = get_page_struct(addr);

    if (pg->is_free) {
        spin_lock(&uart_lock);
        uart_puts("Double free detected at: ");
        uart_puthex((ulong)addr);
        uart_putc('\n');
        spin_unlock(&uart_lock);
        while (1)
            asm volatile("wfi");
    }

    int order = pg->order;
    pg->is_free = true;

    while (order < MAX_ORDER) {
        void *buddy_addr = buddy_of(addr, order);
        page_t *buddy = get_page_struct(buddy_addr);

        if (!buddy->is_free || buddy->order != order)
            break;

        // Remove buddy from freelist
        page_t **p = &free_lists[order];
        while (*p && *p != buddy)
            p = &(*p)->next;

        if (*p == buddy)
            *p = buddy->next;

        buddy->is_free = false;
        if ((ulong)addr > (ulong)buddy_addr)
            addr = buddy_addr;

        order++;
    }

    page_t *merged = get_page_struct(addr);
    merged->order = order;
    merged->is_free = true;
    merged->next = free_lists[order];
    free_lists[order] = merged;

    // Unlock spinlock
    spin_unlock(&buddy_lock);
}
