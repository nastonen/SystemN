.section .text
.global _entry
_entry:
    # Set up a stack for C
    # sp = _stack + ((hartId + 1) * 4096)
    csrr  t0, mhartid
    addi  t0, t0, 1
    li    t1, 4096
    mul   t0, t0, t1
    la    t1, _stack
    add   sp, t1, t0

    call start

spin:
    j spin

# Stack section: 4KB × 4 harts = 16KB
.section .bss
.align 16
.global _stack
_stack:
    .space 4096 * 4
