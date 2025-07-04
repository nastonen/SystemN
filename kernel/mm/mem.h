#pragma once

#include "../types.h"
#include "../string.h"
#include "mapping.h"

#define PAGE_SIZE           4096

#define VA_OFFSET           (KERNEL_START_VA - KERNEL_START)

#define PHYS_TO_VIRT(pa)    ((void *)((ulong)(pa) + va_offset))
#define VIRT_TO_PHYS(va)    ((ulong)(va) - va_offset)

#define KSTACK_SIZE         PAGE_SIZE
#define USER_STACK_SIZE     PAGE_SIZE
#define USER_STACK_TOP      USER_END_VA
#define USER_STACK_BASE     (USER_END_VA - USER_STACK_SIZE)
#define USER_HEAP_END       (USER_STACK_BASE - PAGE_SIZE)                   // Leave 4KB guard

#define MAX_ORDER           10                                              // Max 2^10 pages = 4MB
#define PAGE_ENTRIES        (PAGE_SIZE / sizeof(pte_t))                     // 4096 / 8 = 512
#define ALIGN_UP(addr)      (((addr) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

// Extract physical address from a PTE and vice versa
#define PAGE_SHIFT          12
#define PTE2PA(pte)         (((pte) >> 10) << 12)
#define PA2PTE(pa)          (((ulong)(pa) >> PAGE_SHIFT) << 10)

// Extract permission/flag bits from a PTE
#define PTE_FLAGS(pte)      ((pte) & 0x3FF)  // bits [9:0]

typedef struct page {
    uchar order;            // Power-of-two order (0–10)
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
