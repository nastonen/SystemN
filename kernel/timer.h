#pragma once

#define TIMER_INTERVAL 1000000UL

static inline void
timer_init()
{
    //spin_lock(&uart_lock);
    //uart_puts("Timer inited\n");
    //spin_unlock(&uart_lock);

    write_csr(stimecmp, read_csr(time) + TIMER_INTERVAL);
}

static inline void
timer_handle()
{
    //spin_lock(&uart_lock);
    //uart_puts("Timer handled\n");
    //spin_unlock(&uart_lock);

    write_csr(stimecmp, read_csr(time) + TIMER_INTERVAL);
}
