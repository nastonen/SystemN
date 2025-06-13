#pragma once

#include "../types.h"
#include "../string.h"

#define PAGE_SIZE           4096

#define KERNEL_START        0x80000000
#define KERNEL_END          0x80800000  // 8MB for kernel space
#define USER_START          KERNEL_END
#define USER_END            0x88000000  // 120MB for user space

#define VA_OFFSET           0x40000000

#define KERNEL_START_VA     0xC0000000  // Map kernel to high virtual memory
#define KERNEL_END_VA       0xC0800000  // 8MB for kernel space
#define USER_START_VA              0x0
#define USER_END_VA          0x7800000  // 120MB for user space

#define USER_STACK_SIZE     PAGE_SIZE
#define USER_STACK_TOP      USER_END_VA
#define USER_STACK_BASE     (USER_END_VA - USER_STACK_SIZE)

//#define BUDDY_BASE_PHYS     0x80019000 // Max kernel size = 0x19000 (100KB)
//#define BUDDY_BASE_VA       0xC0019000 // Max kernel size = 0x19000 (100KB)

#define MAX_ORDER           10         // Max 2^10 pages = 4MB

//#define NUM_PAGES           ((USER_END - BUDDY_BASE_PHYS) / PAGE_SIZE)
#define PAGE_ENTRIES        (PAGE_SIZE / sizeof(pte_t)) // 4096 / 8 = 512

#define ALIGN_UP(val)       (((val) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

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
extern ulong buddy_base_phys;
