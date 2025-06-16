#include "snub.h"

static snub_cache_t caches[SNUB_NUM_CLASSES];

static inline int
size_to_idx(ulong size)
{
    int shift = SNUB_MIN_SHIFT;
    while ((1UL << shift) < size && shift <= SNUB_MAX_SHIFT)
        shift++;

    return (shift > SNUB_MAX_SHIFT) ? -1 : (shift - SNUB_MIN_SHIFT);
}

void
snub_init(void)
{
    for (int i = 0; i < SNUB_NUM_CLASSES; i++) {
        caches[i].object_size = 1 << (SNUB_MIN_SHIFT + i);
        caches[i].partial = NULL;
        spinlock_init(&caches[i].lock);
    }
}

void *
kmalloc(ulong size)
{
    int idx = size_to_idx(size);
    if (idx < 0 || idx >= SNUB_NUM_CLASSES)
        return NULL;

    snub_cache_t *cache = &caches[idx];

    // Lock spinlock
    spin_lock(&cache->lock);

    if (!cache->partial) {
        // Allocate new snub page
        void *page = alloc_page();
        if (!page) {
            // Unlock spinlock
            spin_unlock(&cache->lock);
            return NULL;
        }

        int objs_per_page = PAGE_SIZE / cache->object_size;
        void *head = NULL;

        for (int i = 0; i < objs_per_page; i++) {
            void *obj = (char *)page + i * cache->object_size;
            *(void **)obj = head;
            head = obj;
        }

        cache->partial = head;
    }

    void *obj = cache->partial;
    cache->partial = *(void **)obj;

    // Unlock spinlock
    spin_unlock(&cache->lock);

    return obj;
}

void *
kzalloc(ulong size)
{
    void *ptr = kmalloc(size);
    if (ptr)
        memset(ptr, 0, size);

    return ptr;
}

void
kfree(void *ptr)
{
    ulong addr = (ulong)ptr;
    ulong offset = addr - buddy_base_phys;
    int page_idx = offset / PAGE_SIZE;
    ulong page_base = buddy_base_phys + (page_idx * PAGE_SIZE);
    ulong diff = addr - page_base;

    // Guess object size by nearest size class
    for (int i = 0; i < SNUB_NUM_CLASSES; i++) {
        int obj_size = caches[i].object_size;
        if (diff % obj_size == 0 && diff < PAGE_SIZE) {
            // Lock spinlock
            spin_lock(&caches[i].lock);

            *(void **)ptr = caches[i].partial;
            caches[i].partial = ptr;

            // Unlock spinlock
            spin_unlock(&caches[i].lock);

            return;
        }
    }

    // Not a known cache — maybe full page allocation
    free_pages(ptr);
}
