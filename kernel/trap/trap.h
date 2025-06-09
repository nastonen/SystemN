#pragma once

#include "../types.h"
#include "../proc.h"

#define SCAUSE_IRQ_BIT           (1UL << 63)
#define SCAUSE_CODE(scause)      ((scause) & ~SCAUSE_IRQ_BIT)

#define SCAUSE_SUPERVISOR_IRQ    1  // Supervisor software interrupt
#define SCAUSE_ILLEGAL_INSTR     2  // Illegal instruction
#define SCAUSE_TIMER_INTERRUPT   5  // Timer interrupt
#define SCAUSE_USER_ECALL        8  // U-mode system call
#define SCAUSE_SUPERVISOR_ECALL  9  // S-mode system call

void trap_handler(trap_frame_t *tf);
void trap_vector();                 // asm entry point
void idle_trap_vector();            // idle entry point
