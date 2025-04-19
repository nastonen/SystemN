#pragma once

#include "../types.h"

typedef struct trap_frame {
    ulong regs[32];   // x0–x31
    ulong sepc;
    ulong sstatus;
    ulong scause;
    ulong stval;
} trap_frame_t;

void s_trap_handler();
void s_trap_vector();  // ASM entry point
