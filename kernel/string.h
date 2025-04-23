#pragma once

#include "types.h"

static inline void *
memcpy(void *dest, const void *src, ulong n)
{
    char *d = dest;
    const char *s = src;

    while (n--)
        *d++ = *s++;

    return dest;
}
