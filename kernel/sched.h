#pragma once

#include "spinlock.h"

extern spinlock_t sched_lock;

void schedule();
