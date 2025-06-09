.section .text
.global swtch
swtch:
    # Save ra, sp, s0-s11 into old
    sd ra, 0(a0)
    sd sp, 8(a0)
    sd s0, 16(a0)
    sd s1, 24(a0)
    sd s2, 32(a0)
    sd s3, 40(a0)
    sd s4, 48(a0)
    sd s5, 56(a0)
    sd s6, 64(a0)
    sd s7, 72(a0)
    sd s8, 80(a0)
    sd s9, 88(a0)
    sd s10, 96(a0)
    sd s11, 104(a0)

    #srli    t0, a2, 12             # convert to physical PPN (assuming identity mapping)
    #li      t1, 8
    #slli    t1, t1, 60             # t1 = SATP_MODE_SV39 (8 << 60)
    #or      t0, t0, t1             # t0 = final satp value
    #sfence.vma zero, zero          # flush TLB
    #csrw    satp, t0               # load satp
    #sfence.vma zero, zero          # flush TLB

    # Load ra, sp, s0-s11 from new
    ld ra, 0(a1)
    ld sp, 8(a1)
    ld s0, 16(a1)
    ld s1, 24(a1)
    ld s2, 32(a1)
    ld s3, 40(a1)
    ld s4, 48(a1)
    ld s5, 56(a1)
    ld s6, 64(a1)
    ld s7, 72(a1)
    ld s8, 80(a1)
    ld s9, 88(a1)
    ld s10, 96(a1)
    ld s11, 104(a1)

    # return to ra from new context
    ret
