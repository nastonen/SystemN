#pragma once

#include "riscv.h"
#include "trap/trap.h"

#define NCPU   4  // number of CPUs
#define NPROC 64  // max number of processes

enum proc_state {
    UNUSED,
    SLEEPING,
    RUNNABLE,
    RUNNING,
    ZOMBIE
};

struct proc {
    int pid;
    enum proc_state state;
    trap_frame_t tf;
};

extern struct proc proc_table[NPROC];

struct cpu {
    uint id;            // hard ID
    uint lock_depth;    // depth of nested spinlocks
    ulong sstatus;      // interrupt state before first lock
    struct proc *proc;  // current process
};

extern struct cpu cpus[NCPU];

static inline struct cpu *
curr_cpu()
{
    return (struct cpu*)read_tp();
}
