#include "mem.h"
#include "../spinlock.h"

static spinlock_t buddy_lock = SPINLOCK_INIT;
static page_t page_array[NUM_PAGES];
static page_t *free_lists[MAX_ORDER + 1];

static inline ulong
page_idx(void *addr)
{
    return ((ulong)addr - PHYS_BASE) / PAGE_SIZE;
}

static inline page_t *
get_page_struct(void *addr)
{
    return &page_array[page_idx(addr)];
}

static inline void *
page_addr(ulong idx)
{
    return (void *)(PHYS_BASE + idx * PAGE_SIZE);
}

static void *
buddy_of(void *addr, int order)
{
    ulong block = (ulong)addr - PHYS_BASE;
    ulong size = PAGE_SIZE << order;
    ulong buddy = block ^ size;

    return (void *)(buddy + PHYS_BASE);
}

void
buddy_allocator_init(void *start, void *end)
{
    ulong curr = (ulong)start;
    ulong limit = (ulong)end;

    for (int i = 0; i <= MAX_ORDER; i++)
        free_lists[i] = NULL;

    while (curr + PAGE_SIZE <= limit) {
        int order = MAX_ORDER;
        // If not enough space in this order or not aligned to power-of-two,
        // reduce order
        while (order > 0 && (curr + (PAGE_SIZE << order) > limit || curr & ((PAGE_SIZE << order) - 1)))
            order--;

        page_t *pg = get_page_struct((void *)curr);
        pg->order = order;
        pg->is_free = true;
        pg->next = free_lists[order];
        free_lists[order] = pg;

        curr += PAGE_SIZE << order;
    }
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
        for (int curr = o; curr > order; curr--) {
            //ulong buddy_addr = (ulong)page_addr(page_idx(block)) + (PAGE_SIZE << (curr - 1));
            //page_t *buddy = get_page_struct((void *)buddy_addr);
            int buddy_idx = page_idx(page_addr(page_idx(block))) + (1 << (curr - 1));
            page_t *buddy = &page_array[buddy_idx];

            buddy->order = curr - 1;
            buddy->is_free = true;
            buddy->next = free_lists[curr - 1];
            free_lists[curr - 1] = buddy;
        }

        // Zero returned memory to avoid leaking kernel data
        void *addr = page_addr(page_idx(block));
        memset(addr, 0, PAGE_SIZE << order);

        // Unlock spinlock
        spin_unlock(&buddy_lock);

        return addr;
    }

    // Unlock spinlock
    spin_unlock(&buddy_lock);

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
