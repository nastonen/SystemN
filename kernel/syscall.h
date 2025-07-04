#pragma once

#include "types.h"
#include "proc.h"

long sys_write(int fd, void *user_buf, uint len);
long sys_read(int fd, void *user_buf, uint len);
long sys_sleep_ms(cpu_t *c, proc_t *p, ulong ms);
long sys_sbrk(proc_t *p, long size);
long sys_munmap(proc_t *p, ulong addr, ulong size);
