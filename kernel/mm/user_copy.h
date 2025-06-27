#pragma once

#include "mem.h"

static inline uint
is_user_addr(ulong va, uint len)
{
    return len && (va < USER_END_VA) && (len < (USER_END_VA - va));
}

static inline int
copy_from_user(void *kernel_dst, const void *user_src, uint len)
{
    if (!is_user_addr((ulong)user_src, len))
        return -1;

    set_csr(sstatus, SSTATUS_SUM);
    __builtin_memcpy(kernel_dst, user_src, len);
    clear_csr(sstatus, SSTATUS_SUM);

    return 0;
}

static inline int
copy_to_user(void *user_dst, const void *kernel_src, uint len)
{
    if (!is_user_addr((ulong)user_dst, len))
        return -1;

    set_csr(sstatus, SSTATUS_SUM);
    __builtin_memcpy(user_dst, kernel_src, len);
    clear_csr(sstatus, SSTATUS_SUM);

    return 0;
}
