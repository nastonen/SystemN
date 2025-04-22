#.section .data
#.global debug_a7
#debug_a7:
#    .quad 0

.section .text
.global s_trap_vector
s_trap_vector:
    # Save a7 (x17) for debug BEFORE anything else
    #la t0, debug_a7
    #sd x17, 0(t0)

    # Full trap_frame_t = 32 regs + 4 CSRs = 36 * 8 = 288 bytes
    addi sp, sp, -288

    # Save x0–x31 into regs[0..31] (x0 is hardwired, save 0 for consistency)
    sd x0,    0(sp)     # x0 (hardwired zero)
    sd x1,    8(sp)     # ra
    sd x2,   16(sp)     # sp
    sd x3,   24(sp)     # gp
    sd x4,   32(sp)     # tp
    sd x5,   40(sp)     # t0
    sd x6,   48(sp)
    sd x7,   56(sp)
    sd x8,   64(sp)
    sd x9,   72(sp)
    sd x10,  80(sp)
    sd x11,  88(sp)
    sd x12,  96(sp)
    sd x13, 104(sp)
    sd x14, 112(sp)
    sd x15, 120(sp)
    sd x16, 128(sp)
    sd x17, 136(sp)
    sd x18, 144(sp)
    sd x19, 152(sp)
    sd x20, 160(sp)
    sd x21, 168(sp)
    sd x22, 176(sp)
    sd x23, 184(sp)
    sd x24, 192(sp)
    sd x25, 200(sp)
    sd x26, 208(sp)
    sd x27, 216(sp)
    sd x28, 224(sp)
    sd x29, 232(sp)
    sd x30, 240(sp)
    sd x31, 248(sp)

    # Save CSRs at end of struct
    csrr t0, sepc
    sd t0, 256(sp)
    csrr t0, sstatus
    sd t0, 264(sp)
    csrr t0, scause
    sd t0, 272(sp)
    csrr t0, stval
    sd t0, 280(sp)

    # Call trap handler with trap_frame_t* in a0
    mv a0, sp
    call s_trap_handler

    # Restore x-registers x0–x31
    # x0 restore is unnecessary but fine if it matches struct
    ld x1,    8(sp)
    ld x2,   16(sp)
    ld x3,   24(sp)
    ld x4,   32(sp)
    ld x5,   40(sp)
    ld x6,   48(sp)
    ld x7,   56(sp)
    ld x8,   64(sp)
    ld x9,   72(sp)
    ld x10,  80(sp)
    ld x11,  88(sp)
    ld x12,  96(sp)
    ld x13, 104(sp)
    ld x14, 112(sp)
    ld x15, 120(sp)
    ld x16, 128(sp)
    ld x17, 136(sp)
    ld x18, 144(sp)
    ld x19, 152(sp)
    ld x20, 160(sp)
    ld x21, 168(sp)
    ld x22, 176(sp)
    ld x23, 184(sp)
    ld x24, 192(sp)
    ld x25, 200(sp)
    ld x26, 208(sp)
    ld x27, 216(sp)
    ld x28, 224(sp)
    ld x29, 232(sp)
    ld x30, 240(sp)
    ld x31, 248(sp)

    # Restore sepc/sstatus
    ld t0, 256(sp)
    csrw sepc, t0
    ld t0, 264(sp)
    csrw sstatus, t0

    addi sp, sp, 288
    sret
