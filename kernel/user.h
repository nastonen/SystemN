#pragma once

#include "syscall.h"

static inline long
write(const void *buf, unsigned long cnt)
{
    // only stdout for now
    return syscall3(SYS_write, 1, (long)buf, cnt);
}

static inline void
exit(int status)
{
    syscall1(SYS_exit, status);
    //while (1) {} // Should never return
}

static inline long
getpid()
{
    return syscall0(SYS_getpid);
}

static inline void
yield()
{
    syscall0(SYS_yield);
}

static inline long
read(void *buf, unsigned long cnt)
{
    return syscall3(SYS_read, 0, (long)buf, cnt);
}

static inline long
sleep(unsigned long ms)
{
    if (ms == 0 || ms > (60 * 1000))
        return 0;

    return syscall1(SYS_sleep_ms, ms);
}
