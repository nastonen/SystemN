#pragma once

#include "types.h"

typedef struct {
    volatile uint locked;
} spinlock_t;

#define SPINLOCK_INIT { .locked = 0 }

static inline void spinlock_init(spinlock_t *lock) {
    lock->locked = 0;
}

void spin_lock(spinlock_t *lock);
int spin_trylock(spinlock_t *lock);
void spin_unlock(spinlock_t *lock);
