#pragma once

#include "../types.h"
#include "../string.h"

#define PAGE_SIZE           4096

#define KERNEL_START        0x80000000
#define KERNEL_END          0x80800000                                      // 8MB for kernel space
#define USER_START          KERNEL_END
#define USER_END            0x88000000                                      // 120MB for user space

#define KERNEL_START_VA     0xffffffff80000000                              // Map kernel to high virtual memory
#define KERNEL_END_VA       0xffffffff80800000                              // 8MB for kernel space
#define USER_START_VA       0x0
#define USER_END_VA         0x7800000                                       // 120MB for user space


#define KSTACK_SIZE         PAGE_SIZE
#define KSTACK_BASE_VA      0xffffffc000000000                              // Process kernel stacks live here
#define PER_PROC_KSTACK_VA(pid) (KSTACK_BASE_VA + ((pid) * KSTACK_SIZE))


#define VA_OFFSET           (KERNEL_START_VA - KERNEL_START)

#define PHYS_TO_VIRT(pa)    ((void *)((ulong)(pa) + va_offset))
#define VIRT_TO_PHYS(va)    ((ulong)(va) - va_offset)

#define USER_STACK_SIZE     PAGE_SIZE
#define USER_STACK_TOP      USER_END_VA
#define USER_STACK_BASE     (USER_END_VA - USER_STACK_SIZE)

#define MAX_ORDER           10                                              // Max 2^10 pages = 4MB
#define PAGE_ENTRIES        (PAGE_SIZE / sizeof(pte_t))                     // 4096 / 8 = 512
#define ALIGN_UP(addr)      (((addr) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

// Extract physical address from a PTE
#define PTE2PA(pte)     (((pte) >> 10) << 12)

// Extract permission/flag bits from a PTE
#define PTE_FLAGS(pte)  ((pte) & 0x3FF)  // bits [9:0]

typedef struct page {
    uchar order;                                                            // Power-of-two order (0–10)
    uchar is_free;
    struct page *next;
} page_t;

int buddy_allocator_init();
void *alloc_page(void);
void *alloc_pages(int order);
void free_page(void *ptr);
void free_pages(void *addr);

extern pte_t *kernel_pagetable;
extern ulong buddy_base_phys;
extern ulong va_offset;
