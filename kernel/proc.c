#include "proc.h"

struct cpu cpus[NCPU];
struct proc idle_procs[NCPU];

void
idle_loop()
{
    while (1)
        asm volatile("wfi");  // Wait for interrupt
}
