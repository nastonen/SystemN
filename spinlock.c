#include "spinlock.h"

void
spin_lock(spinlock_t *lock)
{
    while (__sync_lock_test_and_set(&lock->locked, 1))
        ;
}

void
spin_unlock(spinlock_t *lock)
{
    __sync_lock_release(&lock->locked);
}
