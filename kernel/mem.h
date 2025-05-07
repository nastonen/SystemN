#pragma once

#include "types.h"

#define KERNEL_START   0x80000000
#define KERNEL_END     0x84000000

#define USER_START     0x84000000
#define USER_END       0x88000000

#define PAGE_SIZE 4096

typedef struct page {
    struct page *next;
} page_t;

void *kmalloc(uint size);
void *kzalloc(uint size);
void  kfree(void *ptr);

void init_allocator(void *mem_start, void *mem_end);
void *alloc_page();
void free_page(void *ptr);
