#pragma once

#define KERNEL_START        0x80000000
#define KERNEL_END          0x80800000                                      // 8MB for kernel space
#define USER_START          KERNEL_END
#define USER_END            0x88000000                                      // 120MB for user space

#define KERNEL_START_VA     0xffffffff80000000                              // Map kernel to high virtual memory
#define KERNEL_END_VA       0xffffffff80800000                              // 8MB for kernel space
#define USER_START_VA       0x0
#define USER_END_VA         0x7800000                                       // 120MB for user space

#define VA_OFFSET           (KERNEL_START_VA - KERNEL_START)
