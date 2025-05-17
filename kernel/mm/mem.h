#pragma once

#include "../types.h"
#include "../string.h"

#define KERNEL_START    0x80000000
#define KERNEL_END      0x84000000

#define USER_START      0x84000000
#define USER_END        0x88000000

#define USER_STACK_SIZE 4096
#define USER_STACK_TOP (USER_START + USER_STACK_SIZE)

//#define PHYS_BASE       KERNEL_START
//#define MAX_PHYS_MEM     0x8000000  // 128MB

//extern char _kernel_end[];
//#define ALIGN_UP(x, align) (((x) + ((align) - 1)) & ~((align) - 1))
//#define BUDDY_BASE_PHYS ALIGN_UP((ulong)_kernel_end, PAGE_SIZE)
#define BUDDY_BASE_PHYS 0x80400000 // max kernel size = 0x400000 (4MB)

#define MAX_ORDER       10          // Max 2^10 pages = 4MB
#define PAGE_SIZE       4096
#define NUM_PAGES       ((KERNEL_END - BUDDY_BASE_PHYS) / PAGE_SIZE)

#define PAGE_ENTRIES  (PAGE_SIZE / sizeof(pte_t)) // 4096 / 8 = 512
//#define KERNEL_PAGETABLE_ADDR  (ALIGN_UP((ulong)_kernel_end, PAGE_SIZE))

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
