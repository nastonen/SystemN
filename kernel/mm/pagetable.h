#pragma once

#include "../types.h"
#include "mem.h"

#define PTE_COUNT           512  // Entries per page table level (9 bits)

#define SATP_MODE_SV39      (8UL << 60)
#define MAKE_SATP(pgtbl)    (SATP_MODE_SV39 | (((ulong)pgtbl >> PAGE_SHIFT) & 0xFFFFFFFFFFFUL))

#define PTE_V (1L << 0)
#define PTE_R (1L << 1)
#define PTE_W (1L << 2)
#define PTE_X (1L << 3)
#define PTE_U (1L << 4)
#define PTE_A (1L << 6)
#define PTE_D (1L << 7)

pte_t *alloc_pagetable(void);
pte_t *walk(pte_t *pagetable, ulong va, int alloc);
int map_page(pte_t *pagetable, ulong va, ulong pa, int perm);
int unmap_page(pte_t *pagetable, ulong va);
void load_pagetable(pte_t *pagetable);
void free_pagetable(pte_t *pagetable);
