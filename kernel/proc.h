#pragma once

#include "riscv.h"
#include "list.h"

#define NCPU            4  // number of CPUs
#define KSTACK_SIZE     4096

// Extract physical address from a PTE
#define PTE2PA(pte)     (((pte) >> 10) << 12)

// Extract permission/flag bits from a PTE
#define PTE_FLAGS(pte)  ((pte) & 0x3FF)  // bits [9:0]

typedef struct trap_frame {
    ulong regs[32];     // x0 - x31
    ulong sepc;         // saved program counter
    ulong sstatus;      // status register
    ulong scause;       // cause of trap
    ulong stval;        // trap value (like faulting addr)
} trap_frame_t;

typedef struct context {
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
    char *kstack;
    ulong sleep_until;
    trap_frame_t *tf;
    context_t ctx;
    list_node_t q_node;
    pte_t *pagetable;
} proc_t;

typedef struct cpu {
    uint id;            // hard ID
    uint lock_depth;    // depth of nested spinlocks
    ulong sstatus;      // interrupt state before first lock
    int needs_sched;
    proc_t *proc;       // current process
    list_node_t run_queue;
    list_node_t sleep_queue;
} cpu_t;

static inline cpu_t *
curr_cpu()
{
    return (cpu_t *)read_tp();
}

extern cpu_t cpus[NCPU];
extern proc_t idle_procs[NCPU];
extern char idle_stack[NCPU][KSTACK_SIZE];

void idle_loop();
proc_t *create_proc(void *binary, ulong binary_size);
void free_proc(proc_t *p);
