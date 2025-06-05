.section .text
.global userret
userret:
    # a0 holds trap_frame_t*

    # Restore x1–x31 (x0 is hardwired to zero)
    ld x1,    8(a0)
    ld x2,   16(a0)
    ld x3,   24(a0)
    ld x4,   32(a0)
    ld x5,   40(a0)
    ld x6,   48(a0)
    ld x7,   56(a0)
    ld x8,   64(a0)
    ld x9,   72(a0)
    ld x10,  80(a0)
    ld x11,  88(a0)
    ld x12,  96(a0)
    ld x13, 104(a0)
    ld x14, 112(a0)
    ld x15, 120(a0)
    ld x16, 128(a0)
    ld x17, 136(a0)
    ld x18, 144(a0)
    ld x19, 152(a0)
    ld x20, 160(a0)
    ld x21, 168(a0)
    ld x22, 176(a0)
    ld x23, 184(a0)
    ld x24, 192(a0)
    ld x25, 200(a0)
    ld x26, 208(a0)
    ld x27, 216(a0)
    ld x28, 224(a0)
    ld x29, 232(a0)
    ld x30, 240(a0)
    ld x31, 248(a0)

    # Restore sepc and sstatus
    ld t0, 256(a0)
    csrw sepc, t0
    ld t0, 264(a0)
    csrw sstatus, t0

    # Return to user mode
    sret
