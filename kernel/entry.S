.section .text
.global _entry
_entry:
    # set up a stack for C.
    # sp = stack_end + (hartId * 4096)
    csrr  t0, mhartid
    li    t1, 4096
    la    t2, stack_end
    mul   t3, t0, t1
    sub   sp, t2, t3

    call start

spin:
    j spin

# Stack section: 4KB × 4 harts = 16KB
.section .bss
.align 4
.global stack
stack:
    .space 4096 * 4

.global stack_end
stack_end:
