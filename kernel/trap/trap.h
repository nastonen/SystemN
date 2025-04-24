#pragma once

#include "../types.h"

#define SCAUSE_IRQ_BIT           (1UL << 63)
#define SCAUSE_CODE(scause)      ((scause) & ~SCAUSE_IRQ_BIT)
#define SCAUSE_USER_ECALL        8  // U-mode system call
#define SCAUSE_SUPERVISOR_ECALL  9  // S-mode system call
#define SCAUSE_TIMER_INTERRUPT   5  // Timer interrupt

typedef struct trap_frame {
    ulong regs[32];     // x0 - x31
    ulong sepc;         // saved program counter
    ulong sstatus;      // status register
    ulong scause;       // cause of trap
    ulong stval;        // trap value (like faulting addr)
} trap_frame_t;

void s_trap_handler(trap_frame_t *tf);
void s_trap_vector();   // ASM entry point
