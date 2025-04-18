#ifndef SPINLOCK_H
#define SPINLOCK_H

#include "types.h"

typedef struct {
    volatile uint locked;
} spinlock_t;

#define SPINLOCK_INIT { .locked = 0 }


void spin_lock(spinlock_t *lock);
void spin_unlock(spinlock_t *lock);

#endif
