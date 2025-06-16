#include "mem.h"
#include "../spinlock.h"
#include "../uart.h"

extern char _kernel_end[];
static spinlock_t buddy_lock = SPINLOCK_INIT;
static ulong num_pages;
static page_t *page_array;
static page_t *free_lists[MAX_ORDER + 1];
ulong buddy_base_phys;
pte_t *kernel_pagetable; //[PAGE_ENTRIES];

page_t *
get_page_struct(void *pa)
{
    ulong idx = ((ulong)pa - buddy_base_phys) / PAGE_SIZE;
    if (idx >= num_pages) {
        uart_puts("get_page_struct: INVALID idx: ");
        uart_putlong(idx);
        uart_putc('\n');
        while (1)
            asm volatile("wfi");
    }
    return &page_array[idx];
}


static inline void *
page_addr(ulong idx)
{
    if (idx >= num_pages) {
        spin_lock(&uart_lock);
        uart_puts("page_addr: INVALID IDX ");
        uart_putlong(idx);
        uart_putc('\n');
        spin_unlock(&uart_lock);
        while (1)
            asm volatile("wfi");
    }
    return (void *)(buddy_base_phys + idx * PAGE_SIZE);
}

static void *
buddy_of(void *addr, int order)
{
    ulong block = (ulong)addr - buddy_base_phys;
    ulong size = PAGE_SIZE << order;
    ulong buddy = block ^ size;

    return (void *)(buddy + buddy_base_phys);
}

void *
early_alloc(ulong size)
{
    // Align size
    size = ALIGN_UP(size);

    if (!buddy_base_phys)
        buddy_base_phys = ALIGN_UP((ulong)_kernel_end);

    void *addr = (void *)buddy_base_phys;
    buddy_base_phys += size;
    return addr;
}

int
buddy_allocator_init()
{
    DEBUG_PRINT(
        uart_puts("Initializing buddy allocator...\n");
    );

    // Allocate kernel pagetable
    kernel_pagetable = early_alloc(PAGE_SIZE);
    memset((void *)kernel_pagetable, 0, PAGE_SIZE);

    // Calculate memory range
    ulong phys_start = buddy_base_phys;
    ulong phys_end = USER_END;

    // Estimate max pages, accounting for metadata per page
    ulong per_page_cost = PAGE_SIZE + sizeof(page_t);
    ulong estimated_pages = (phys_end - phys_start) / per_page_cost;

    // Allocate page_array metadata
    page_array = early_alloc(estimated_pages * sizeof(page_t));
    memset(page_array, 0, estimated_pages * sizeof(page_t));

    // Align and update the start of page memory
    buddy_base_phys = ALIGN_UP(buddy_base_phys);
    phys_start = buddy_base_phys;

    // Calculate how many pages fit now
    num_pages = (USER_END - buddy_base_phys) / PAGE_SIZE;

    // Initialize free lists
    for (int i = 0; i <= MAX_ORDER; i++)
        free_lists[i] = NULL;

    // Populate initial free blocks
    ulong curr = phys_start;
    while (curr + PAGE_SIZE <= phys_end) {
        int order = MAX_ORDER;

        // Align block and ensure space
        while (order > 0 &&
               (curr + (PAGE_SIZE << order) > phys_end ||
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

    DEBUG_PRINT(
        uart_puts("Buddy allocator ready.\n");
    );

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
            if (buddy_idx >= num_pages) {
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
