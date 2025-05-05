#pragma once

#include "riscv.h"

#define NCPU        4  // number of CPUs
#define NPROC       64  // max number of processes
#define KSTACK_SIZE 4096

void idle_loop();

static inline struct cpu *
curr_cpu()
{
    return (struct cpu*)read_tp();
}

typedef struct trap_frame {
    ulong regs[32];     // x0 - x31
    ulong sepc;         // saved program counter
    ulong sstatus;      // status register
    ulong scause;       // cause of trap
    ulong stval;        // trap value (like faulting addr)
} trap_frame_t;

typedef struct context_t {
    ulong ra;
    ulong sp;
    ulong s[12];
} context_t;

enum proc_state {
    UNUSED,
    SLEEPING,
    RUNNABLE,
    RUNNING,
    ZOMBIE
};

typedef struct proc {
    int pid;
    enum proc_state state;
    int is_idle;
    int bound_cpu;
    char kstack[KSTACK_SIZE];
    ulong sleep_until;
    trap_frame_t *tf;
    context_t ctx;
} proc_t;


struct cpu {
    uint id;            // hard ID
    uint lock_depth;    // depth of nested spinlocks
    ulong sstatus;      // interrupt state before first lock
    int needs_sched;
    struct proc *proc;  // current process
};

extern struct cpu cpus[NCPU];
extern struct proc proc_table[NPROC];

extern struct proc boot_procs[NCPU];
extern char boot_stack[NCPU][KSTACK_SIZE];

extern struct proc idle_procs[NCPU];
extern char idle_stack[NCPU][KSTACK_SIZE];
