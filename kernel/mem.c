#include "mem.h"
#include "types.h"
#include "string.h"

static page_t *free_list = 0;

void
init_allocator(void *mem_start, void *mem_end)
{
    char *p = (char *)mem_start;

    while (p + PAGE_SIZE <= (char *)mem_end) {
        page_t *pg = (page_t *)p;
        pg->next = free_list;
        free_list = pg;
        p += PAGE_SIZE;
    }
}

void *
alloc_page()
{
    if (!free_list)
        return 0;

    page_t *page = free_list;
    free_list = free_list->next;

    return (void *)page;
}

void
free_page(void *ptr)
{
    page_t *page = (page_t *)ptr;
    page->next = free_list;
    free_list = page;
}

void *
kmalloc(uint size)
{
    if (size == 0)
        return 0;

    uint num_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;

    // Allocate contiguous pages
    void *first = 0;
    void *prev = 0;

    for (uint i = 0; i < num_pages; i++) {
        void *page = alloc_page();
        if (!page) {
            // Roll back on failure
            while (first) {
                void *next = *((void **)first);
                free_page(first);
                first = next;
            }
            return 0;
        }

        if (prev)
            *((void **)prev) = page;
        else
            first = page;

        prev = page;
    }

    // NULL-terminate the chain
    *((void **)prev) = 0;

    return first;
}

void *
kzalloc(uint size)
{
    void *mem = kmalloc(size);

    if (mem)
        memset(mem, 0, size);

    return mem;
}

void
kfree(void *ptr)
{
    if (ptr)
        free_page(ptr);
}
