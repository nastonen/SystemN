#pragma once

#include "types.h"

#define read_csr(reg) ({                           \
    ulong __tmp;                                   \
    asm volatile("csrr %0, " #reg : "=r"(__tmp));  \
    __tmp;                                         \
})

#define write_csr(reg, val) ({                      \
    ulong __v = (ulong)(val);                       \
    asm volatile("csrw " #reg ", %0" :: "rK"(__v)); \
})

static inline ulong
read_tp()
{
    ulong val;
    asm volatile("mv %0, tp" : "=r"(val));
    return val;
}

static inline void
write_tp(ulong val)
{
    asm volatile("mv tp, %0" :: "r"(val));
}
