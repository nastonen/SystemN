#pragma once

#define NCPU 4

struct cpu {
    int id;                 // hard ID
    int lock_depth;         // depth of nested spinlocks
    ulong sstatus;          // interrupt state before first lock
    //struct proc *prod;    // current process
};

extern struct cpu cpus[NCPU];

static inline struct cpu *
curr_cpu()
{
    return (struct cpu*)read_tp();
}
