#include "pagetable.h"
#include "../riscv.h"
#include "../uart.h"

static inline int
vpn_level(ulong va, int level)
{
    return (va >> (PAGE_SHIFT + 9 * level)) & 0x1FF;
}

/*
 * Virtual Address (Sv39): 39 bits
 *   | 9 bits | 9 bits | 9 bits | 12 bits |
 *   |  VPN2  |  VPN1  |  VPN0  |  Offset |
 *
 * Levels:   L2     ->    L1    ->    L0
 *           pagetable
 */
pte_t *
walk(pte_t *pagetable, ulong va, int alloc)
{
    for (int level = 2; level > 0; level--) {
        uint idx = vpn_level(va, level);
        pte_t *pte = &pagetable[idx];

        if (*pte & PTE_V) {
            pagetable = (pte_t *)PHYS_TO_VIRT(PTE2PA(*pte));
        } else {
            //uart_puts("current level not valid!\n");
            if (!alloc){
                DEBUG_PRINT(uart_puts("walk(): access to pagetable failed\n"););
                return NULL;
            }

            void *new_pg = alloc_page();
            if (!new_pg) {
                DEBUG_PRINT(uart_puts("alloc_page failed in walk()\n"););
                return NULL;
            }

            *pte = PA2PTE(new_pg) | PTE_V;
            pagetable = (pte_t *)PHYS_TO_VIRT(new_pg);
        }
    }

    return &pagetable[vpn_level(va, 0)];
}

int
map_page(pte_t *pagetable, ulong va, ulong pa, int perm)
{
    // Convert to virtual address
    pagetable = PHYS_TO_VIRT(pagetable);

    pte_t *pte = walk(pagetable, va, 1);
    if (!pte)
        return -1;

    if (!(*pte & PTE_V))
        *pte = PA2PTE(pa) | perm | PTE_V | PTE_A | PTE_D;

    return 0;
}

int
unmap_page(pte_t *pagetable, ulong va)
{
    // Convert to virtual address
    pagetable = PHYS_TO_VIRT(pagetable);

    pte_t *pte = walk(pagetable, va, 0);
    if (!pte || !(*pte & PTE_V))
        return -1;

    ulong pa = PTE2PA(*pte);
    free_page((void *)pa);

    // Clear the PTE
    *pte = 0;

    // Flush TLB for this address
    asm volatile("sfence.vma %0, zero" :: "r"(va) : "memory");

    return 0;
}


pte_t *
alloc_pagetable(void)
{
    return (pte_t *)alloc_page();
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
    // Convert to virtual address
    pagetable = PHYS_TO_VIRT(pagetable);

    for (int i = 0; i < PTE_COUNT; i++) {
        pte_t pte = pagetable[i];
        if ((pte & PTE_V) && !(pte & (PTE_R | PTE_W | PTE_X))) {
            pte_t *child = (pte_t *)PTE2PA(pte);
            free_pagetable(child);
        }
    }
    free_page(pagetable);
}
