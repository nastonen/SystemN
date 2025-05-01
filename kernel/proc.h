#pragma once

#include "riscv.h"
#include "trap/trap.h"

#define NCPU        4  // number of CPUs
#define NPROC       64  // max number of processes
#define KSTACK_SIZE 4096

void idle_loop();

static inline struct cpu *
curr_cpu()
{
    return (struct cpu*)read_tp();
}

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

struct proc {
    int pid;
    enum proc_state state;
    int is_idle;
    int bound_cpu;
    char kstack[KSTACK_SIZE];
    trap_frame_t *tf;
    context_t ctx;
};


struct cpu {
    uint id;            // hard ID
    uint lock_depth;    // depth of nested spinlocks
    ulong sstatus;      // interrupt state before first lock
    struct proc *proc;  // current process
};

extern struct cpu cpus[NCPU];
extern struct proc proc_table[NPROC];
extern struct proc idle_procs[NCPU];  // One idle process per CPU
