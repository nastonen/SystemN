#include "spinlock.h"

void
spin_lock(spinlock_t *lock)
{
    struct cpu *c = curr_cpu();

    if (c->lock_depth == 0) {
        // Save current interrupt state and disable interrupts
        c->sstatus = read_csr(sstatus);
        write_csr(sstatus, c->sstatus & ~0x2);
    }
    c->lock_depth++;

    while (__atomic_test_and_set(&lock->locked, __ATOMIC_ACQUIRE))
        asm volatile("pause");
}

void
spin_unlock(spinlock_t *lock)
{
    struct cpu *c = curr_cpu();

    // Decrement lock nesting depth
    c->lock_depth--;

    // Release the lock — __ATOMIC_RELEASE ensures memory ordering
    __atomic_clear(&lock->locked, __ATOMIC_RELEASE);

    // Only restore interrupt state when outermost lock is released
    // Restore sstatus (e.g., re-enable interrupts if they were on before)
    if (c->lock_depth == 0)
        write_csr(sstatus, c->sstatus);
}
