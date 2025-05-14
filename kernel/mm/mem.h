#pragma once

#include "../types.h"
#include "../string.h"

#define KERNEL_START    0x80000000
#define KERNEL_END      0x84000000

#define USER_START      0x84000000
#define USER_END        0x88000000

#define PHYS_BASE       KERNEL_START
#define MAX_PHYS_MEM     0x8000000  // 128MB

#define MAX_ORDER       10          // Max 2^10 pages = 4MB
#define PAGE_SIZE       4096
#define NUM_PAGES       (MAX_PHYS_MEM / PAGE_SIZE)

typedef struct page {
    uchar order;        // Power-of-two order (0–10)
    uchar is_free;
    struct page *next;
} page_t;

void init_buddy_allocator(void *start, void *end);
void *alloc_page(void);
void *alloc_pages(int order);
void free_page(void *ptr);
void free_pages(void *addr);
