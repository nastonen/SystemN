#pragma once

#include "spinlock.h"
#include "mm/pagetable.h"

extern spinlock_t sched_lock;

void schedule();
