#include "pagetable.h"
#include "mem.h"
#include "../riscv.h"
#include "../uart.h"

static inline int
vpn_level(ulong va, int level)
{
    return (va >> (PAGE_SHIFT + 9 * level)) & 0x1FF;
}

pte_t *
walk(pte_t *pagetable, ulong va, int alloc)
{
    for (int level = 2; level > 0; level--) {
        uint idx = vpn_level(va, level);
        if (idx >= PAGE_ENTRIES) {
            uart_puts("vpn index overflow!\n");
            return NULL;
        }
        pte_t *pte = &pagetable[idx];

        if (*pte & PTE_V) {
            pagetable = (pte_t *)(((*pte >> 10) << PAGE_SHIFT));
        } else {
            uart_puts("current level not valid!\n");

            if (!alloc)
                return NULL;

            void *new_pg = alloc_page();
            if (!new_pg) {
                uart_puts("alloc_page failed in walk()\n");
                return NULL;
            }

            *pte = ((ulong)new_pg >> PAGE_SHIFT) << 10 | PTE_V;
            pagetable = (pte_t *)new_pg;
        }
    }

    return &pagetable[vpn_level(va, 0)];
}

int
map_page(pte_t *pagetable, ulong va, ulong pa, int perm)
{
    pte_t *pte = walk(pagetable, va, 1);

    if (!pte)
        return -1;

    if (*pte & PTE_V)
        return -2;  // already mapped

    *pte = ((pa >> PAGE_SHIFT) << 10) | perm | PTE_V | PTE_A | PTE_D;

    return 0;
}

pte_t *
alloc_pagetable(void)
{
    uart_puts("alloc_pagetable()\n");

    pte_t *pt = (pte_t *)alloc_page();
    if (pt)
        memset(pt, 0, PAGE_SIZE);

    return pt;
}

void
load_pagetable(pte_t *pagetable)
{
    write_csr(satp, MAKE_SATP(pagetable));

    // Flush TLB
    asm volatile ("sfence.vma zero, zero");
}

void
free_pagetable(pte_t *pagetable)
{
    for (int i = 0; i < PTE_COUNT; i++) {
        pte_t pte = pagetable[i];
        if ((pte & PTE_V) && !(pte & (PTE_R | PTE_W | PTE_X))) {
            pte_t *child = (pte_t *)((pte >> 10) << PAGE_SHIFT);
            free_pagetable(child);
        }
    }
    free_page(pagetable);
}
