#pragma once

#define SYS_write   64
#define SYS_exit    93
#define SYS_getpid  172

//#define SYS_fork    220
//#define SYS_read    63
//#define SYS_open    102
//#define SYS_close   57
//#define SYS_brk     214

static inline long
syscall(long num, long arg0, long arg1, long arg2,
                  long arg3, long arg4, long arg5)
{
    register long a0 asm("a0") = arg0;
    register long a1 asm("a1") = arg1;
    register long a2 asm("a2") = arg2;
    register long a3 asm("a3") = arg3;
    register long a4 asm("a4") = arg4;
    register long a5 asm("a5") = arg5;
    register long a7 asm("a7") = num;

    asm volatile (
        "ecall"
        : "+r"(a0)
        : "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a7)
        : "memory"
    );

    return a0;
}

static inline long
syscall0(long num)
{
    return syscall(num, 0, 0, 0, 0, 0, 0);
}

static inline long
syscall1(long num, long arg0)
{
    return syscall(num, arg0, 0, 0, 0, 0, 0);
}

static inline long
syscall3(long num, long arg0, long arg1, long arg2)
{
    return syscall(num, arg0, arg1, arg2, 0, 0, 0);
}
