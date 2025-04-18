#include "spinlock.h"

void
spin_lock(spinlock_t *lock)
{
    // __ATOMIC_ACQUIRE ensures no memory reordering before the lock is acquired
    while (__atomic_test_and_set(&lock->locked, __ATOMIC_ACQUIRE))
        asm volatile ("pause");
}

void
spin_unlock(spinlock_t *lock)
{
    // __ATOMIC_RELEASE ensures all writes inside the critical section complete
    // before the lock is released
    __atomic_clear(&lock->locked, __ATOMIC_RELEASE);
}
