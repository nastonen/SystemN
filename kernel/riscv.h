#pragma once

#include "types.h"

#define SSTATUS_SIE         (1UL << 1)
#define SSTATUS_SPP         (1UL << 8)
#define SSTATUS_SPIE        (1UL << 5)
#define SSTATUS_UPIE        (0UL << 8)
#define SSTATUS_SUM         (1UL << 18)

#define SIE_STIE            (1UL << 5)
#define SIE_SEIE            (1UL << 9)

#define MENVCFG_FDT         (1UL << 63)
#define MCOUNTEREN_TIME     (1UL << 1)
#define MSTATUS_MPP_MASK    (3UL << 11)  // Mask for MPP (bits 11-12)
#define MSTATUS_MPP_S_MODE  (1UL << 11)  // MPP set to S-mode (01)

#define read_csr(reg) ({                           \
    ulong __tmp;                                   \
    asm volatile("csrr %0, " #reg : "=r"(__tmp));  \
    __tmp;                                         \
})

#define write_csr(reg, val) ({                      \
    ulong __v = (ulong)(val);                       \
    asm volatile("csrw " #reg ", %0" :: "rK"(__v)); \
})

#define set_csr(reg, bit) ({                        \
    asm volatile("csrs " #reg ", %0" :: "rK"(bit)); \
})

#define clear_csr(reg, bit) ({                      \
    asm volatile("csrc " #reg ", %0" :: "rK"(bit)); \
})

#define ATOMIC_FETCH_AND_INC(ptr) ({   \
    long __old_val;                    \
    asm volatile (                     \
        "li t0, 1\n\t"                 \
        "amoadd.d.aq %0, t0, (%1)\n\t" \
        : "=r"(__old_val)              \
        : "r"(ptr)                     \
        : "t0", "memory");             \
    __old_val;                         \
})

#define ATOMIC_INC_AND_FETCH(ptr) ({        \
    long __old_val;                         \
    asm volatile (                          \
        "li t0, 1\n\t"                      \
        "amoadd.d.aq %0, t0, (%1)\n\t"      \
        : "=r"(__old_val)                   \
        : "r"(ptr)                          \
        : "t0", "memory");                  \
    __old_val + 1;                          \
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
