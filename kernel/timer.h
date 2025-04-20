#pragma once

#define TIMER_INTERVAL 1000000UL

static inline void
timer_init()
{
    write_csr(stimecmp, read_csr(time) + TIMER_INTERVAL);
}

static inline void
timer_handle()
{
    write_csr(stimecmp, read_csr(time) + TIMER_INTERVAL);
}
