#pragma once

#include "../types.h"
#include "../string.h"

#define PAGE_SIZE           4096

#define KERNEL_START        0x80000000
#define KERNEL_END          0x84000000

#define USER_START          0x84000000
#define USER_END            0x88000000

#define USER_STACK_SIZE     PAGE_SIZE
#define USER_STACK_TOP      USER_END
#define USER_STACK_BASE     (USER_END - USER_STACK_SIZE)

#define BUDDY_BASE_PHYS     0x80400000 // max kernel size = 0x400000 (4MB)

#define MAX_ORDER           10         // Max 2^10 pages = 4MB
#define NUM_PAGES           ((KERNEL_END - BUDDY_BASE_PHYS) / PAGE_SIZE)
#define PAGE_ENTRIES        (PAGE_SIZE / sizeof(pte_t)) // 4096 / 8 = 512

typedef struct page {
    uchar order;        // Power-of-two order (0–10)
    uchar is_free;
    struct page *next;
} page_t;

int buddy_allocator_init();
void *alloc_page(void);
void *alloc_pages(int order);
void free_page(void *ptr);
void free_pages(void *addr);

extern pte_t kernel_pagetable[PAGE_ENTRIES];
