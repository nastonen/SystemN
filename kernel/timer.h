#pragma once

#include "sched.h"

#define TIME_FREQ      10000000UL                   // 10 MHz
#define TIMER_INTERVAL (TIME_FREQ / 100)            // 100 Hz
#define MS_TO_TIME(ms) ((ms) * (TIME_FREQ / 1000))
#define MAX_SLEEP_MS   60 * 1000                    // 1 minute

static inline ulong
read_time()
{
    return read_csr(time);
}

static inline void
timer_init()
{
    //spin_lock(&uart_lock);
    //uart_puts("timer_init()\n");
    //spin_unlock(&uart_lock);

    write_csr(stimecmp, read_time() + TIMER_INTERVAL);
}

static inline void
timer_handle()
{
    write_csr(stimecmp, read_time() + TIMER_INTERVAL);

    //spin_lock(&uart_lock);
    //uart_puts("timer_handle()\n");
    //spin_unlock(&uart_lock);

    int sched = 0;
    for (int i = 0; i < NPROC; i++) {
        proc_t *p = &proc_table[i];
        if (p && p->state == SLEEPING && read_time() >= p->sleep_until) {
            spin_lock(&uart_lock);
            uart_puts("Proc id ");
            uart_putc('0' + p->pid);
            uart_puts(" became runnable\n");
            spin_unlock(&uart_lock);

            p->state = RUNNABLE;
            sched = 1;
        }
    }

    if (sched)
        schedule();
}
