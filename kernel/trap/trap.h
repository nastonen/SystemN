#pragma once

#include "../types.h"

typedef struct trap_frame {
    ulong regs[32];     // x0 - x31
    ulong sepc;         // saved program counter
    ulong sstatus;      // status register
    ulong scause;       // cause of trap
    ulong stval;        // trap value (like faulting addr)
} trap_frame_t;

void s_trap_handler(trap_frame_t *tf);
void s_trap_vector();   // ASM entry point
