#include "syscall.h"

static inline int
syscall(int num, ...)
{
    va_list args;
    va_start(args, num);
    register long a0 asm("a0") = va_arg(args, long);  // First argument
    register long a1 asm("a1") = va_arg(args, long);  // Second argument
    register long a2 asm("a2") = va_arg(args, long);  // Third argument
    register long a7 asm("a7") = num;  // Syscall number

    asm volatile("ecall" : "=r"(a0) : "r"(a0), "r"(a1), "r"(a2), "r"(a7));
    va_end(args);

    return a0;  // Return the syscall result
}
