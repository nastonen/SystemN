.section .text
.global s_trap_vector
s_trap_vector:
    # Optional: Save a7 for debug
    # la t0, debug_a7
    # sd x17, 0(t0)

    # Save trap cause early
    csrr t1, scause

    # Check SPP bit in sstatus: 0 = user, 1 = supervisor
    csrr t0, sstatus
    andi t0, t0, (1 << 8)   # Check SPP bit (bit 8)
    beqz t0, .from_user     # If SPP == 0 → from user mode
    j .from_kernel

# ------------------------------
.from_user:
    # Load trap frame base pointer (per-thread) from sscratch
    csrr t0, sscratch

    # Save user sp into trap frame (x2), offset 16
    sd sp, 16(t0)

    # Move trap frame base to sp
    mv sp, t0
    j .save_regs

# ------------------------------
.from_kernel:
    # We're already on kernel stack, use current sp directly
    # Reserve space for trap frame (35 x 8 = 280 bytes)
    addi sp, sp, -288

.save_regs:
    # Save x1–x31 (excluding x0, and x2 already saved if from_user)
    # sd x0     0(sp)     # x0 (hardwired zero)
    sd x1,    8(sp)     # ra
    # x2 (sp) was saved manually (at 16) in .from_user
    sd x3,   24(sp)     # gp
    sd x4,   32(sp)     # tp
    sd x5,   40(sp)     # t0
    sd x6,   48(sp)     # t1
    sd x7,   56(sp)     # t2
    sd x8,   64(sp)     # s0/fp
    sd x9,   72(sp)     # s1
    sd x10,  80(sp)     # a0
    sd x11,  88(sp)     # a1
    sd x12,  96(sp)     # a2
    sd x13, 104(sp)     # a3
    sd x14, 112(sp)     # a4
    sd x15, 120(sp)     # a5
    sd x16, 128(sp)     # a6
    sd x17, 136(sp)     # a7
    sd x18, 144(sp)     # s2
    sd x19, 152(sp)     # s3
    sd x20, 160(sp)     # s4
    sd x21, 168(sp)     # s5
    sd x22, 176(sp)     # s6
    sd x23, 184(sp)     # s7
    sd x24, 192(sp)     # s8
    sd x25, 200(sp)     # s9
    sd x26, 208(sp)     # s10
    sd x27, 216(sp)     # s11
    sd x28, 224(sp)     # t3
    sd x29, 232(sp)     # t4
    sd x30, 240(sp)     # t5
    sd x31, 248(sp)     # t6

    # Save CSRs
    csrr t0, sepc
    sd t0, 256(sp)
    csrr t0, sstatus
    sd t0, 264(sp)
    sd t1, 272(sp)       # scause (already read)
    csrr t0, stval
    sd t0, 280(sp)

    # Pass trap_frame_t* in a0
    mv a0, sp

    # Decide handler
    #csrr t0, sstatus
    #andi t0, t0, (1 << 8)   # Check SPP again
    #beqz t0, .call_user_handler
    #j .call_kernel_handler

#.call_user_handler:
    call s_trap_handler #_user
#    j .restore_regs

#.call_kernel_handler:
#    call s_trap_handler #_kernel
#    j .restore_regs

# ------------------------------
#.restore_regs:
    # sd x0     0(sp)     # x0 (hardwired zero)
    ld x1,    8(sp)     # ra
    # x2 (sp) handle separately
    ld x3,   24(sp)     # gp
    ld x4,   32(sp)     # tp
    ld x5,   40(sp)     # t0
    ld x6,   48(sp)     # t1
    ld x7,   56(sp)     # t2
    ld x8,   64(sp)     # s0/fp
    ld x9,   72(sp)     # s1
    ld x10,  80(sp)     # a0
    ld x11,  88(sp)     # a1
    ld x12,  96(sp)     # a2
    ld x13, 104(sp)     # a3
    ld x14, 112(sp)     # a4
    ld x15, 120(sp)     # a5
    ld x16, 128(sp)     # a6
    ld x17, 136(sp)     # a7
    ld x18, 144(sp)     # s2
    ld x19, 152(sp)     # s3
    ld x20, 160(sp)     # s4
    ld x21, 168(sp)     # s5
    ld x22, 176(sp)     # s6
    ld x23, 184(sp)     # s7
    ld x24, 192(sp)     # s8
    ld x25, 200(sp)     # s9
    ld x26, 208(sp)     # s10
    ld x27, 216(sp)     # s11
    ld x28, 224(sp)     # t3
    ld x29, 232(sp)     # t4
    ld x30, 240(sp)     # t5
    ld x31, 248(sp)     # t6

    # Restore CSRs
    ld t0, 256(sp)
    csrw sepc, t0
    ld t0, 264(sp)
    csrw sstatus, t0

    # If we entered from kernel mode, deallocate trap frame
    csrr t0, sstatus
    andi t0, t0, (1 << 8)
    bnez t0, .restore_kernel_stack

    # Restore user sp (x2)
    ld x2, 16(sp)
    sret

.restore_kernel_stack:
    addi sp, sp, 288
    sret
