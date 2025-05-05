#include "spinlock.h"
#include "proc.h"
#include "uart.h"

void
spin_lock(spinlock_t *lock)
{
    struct cpu *c = curr_cpu();
    if (c->lock_depth == 0) {
        // Save current interrupt state and disable interrupts
        c->sstatus = read_csr(sstatus);
        write_csr(sstatus, c->sstatus & ~(1UL << 1));  // Clear SIE

    }
    c->lock_depth++;

    while (__atomic_test_and_set(&lock->locked, __ATOMIC_ACQUIRE))
        asm volatile("nop");
}

int
spin_trylock(spinlock_t *lock)
{
    struct cpu *c = curr_cpu();
    if (c->lock_depth == 0) {
        // Save current interrupt state and disable interrupts
        c->sstatus = read_csr(sstatus);
        write_csr(sstatus, c->sstatus & ~(1UL << 1));  // Clear SIE

    }
    c->lock_depth++;

    int expected = 0;
    return __atomic_compare_exchange_n(&lock->locked, &expected, 1,
                                       0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED);
}

void
spin_unlock(spinlock_t *lock)
{
    struct cpu *c = curr_cpu();

    // Sanity check: lock should not be unlocked if not locked
    if (c->lock_depth == 0) {
        uart_puts("[panic] spin_unlock: lock_depth underflow\n");
        while (1)
            asm volatile("wfi");
    }

    // Decrement lock nesting depth
    c->lock_depth--;

    // Release the lock — __ATOMIC_RELEASE ensures memory ordering
    __atomic_clear(&lock->locked, __ATOMIC_RELEASE);

    // Only restore interrupt state when outermost lock is released
    // Restore sstatus (e.g., re-enable interrupts if they were on before)
    if (c->lock_depth == 0)
        write_csr(sstatus, c->sstatus);
}
