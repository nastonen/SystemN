#pragma once

#define NULL 0

typedef unsigned int    uint;
typedef unsigned long   ulong;
typedef unsigned short  ushort;
typedef unsigned char   uchar;

#ifdef DEBUG
#define DEBUG_PRINT(...)                    \
    do {                                    \
        spin_lock(&uart_lock);              \
        __VA_ARGS__;                        \
        spin_unlock(&uart_lock);            \
    } while (0)
#else
#define DEBUG_PRINT(...) do {} while (0)
#endif

