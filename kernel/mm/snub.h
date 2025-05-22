#pragma once

#include "../spinlock.h"
#include "../types.h"
#include "mem.h"

#define SNUB_MIN_SHIFT      4   // 2^4 = 16 bytes
#define SNUB_MAX_SHIFT      12  // 2^12 = 4096 bytes
#define SNUB_NUM_CLASSES    (SNUB_MAX_SHIFT - SNUB_MIN_SHIFT + 1)

typedef struct snub_cache {
    int object_size;
    void *partial;          // Freelist of objects
    spinlock_t lock;
} snub_cache_t;

void snub_init(void);
void *kmalloc(ulong size);
void *kzalloc(ulong size);
void kfree(void *ptr);
