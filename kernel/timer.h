#pragma once

#include "sched.h"
#include "list.h"

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
    write_csr(stimecmp, read_time() + TIMER_INTERVAL);
}

static inline void
timer_handle()
{
    write_csr(stimecmp, read_time() + TIMER_INTERVAL);

    //DEBUG_PRINT(
      //  uart_puts("timer_handle()\n");
    //);

    cpu_t *c = curr_cpu();
    int need_sched = 0;
    ulong now = read_time();

    list_node_t *pos, *tmp;
    list_for_each_safe(pos, tmp, &c->sleep_queue) {
        proc_t *p = container_of(pos, proc_t, q_node);
        if (now >= p->sleep_until) {
            list_del(&p->q_node);
            p->state = RUNNABLE;
            list_add_tail(&p->q_node, &c->run_queue);
            need_sched = 1;

            DEBUG_PRINT(
                uart_puts("CPU ");
                uart_putc('0' + curr_cpu()->id);
                uart_puts(": proc id ");
                uart_putc('0' + p->pid);
                uart_puts(" became runnable\n");
            );
        }
    }


    if (need_sched)
        curr_cpu()->needs_sched = 1;
}
