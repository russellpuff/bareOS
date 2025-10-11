	.file "ctxsw.s"
	.option arch, +zicsr
	.equ REGSZ, 8

#  `ctxsw`  takes two arguments, a source  thread and destination thread.  It
#  saves the current  state of the CPU into the  source thread's  table entry
#  then restores the state of the destination thread onto the CPU and returns

# thread_t offsets per IDE-reported memory layout
    .equ THREAD_STACKPTR, 0
    .equ THREAD_ROOTPPN,  8
    .equ THREAD_ASID,     24
	.equ SATP_MODE_SV39,  (8 << 60)

	.equ OFF_RA,      0*REGSZ
    .equ OFF_SSTATUS, 1*REGSZ
    .equ OFF_SEPC,    2*REGSZ
    .equ OFF_A0,      3*REGSZ
    .equ OFF_A1,      4*REGSZ
    .equ OFF_A2,      5*REGSZ
    .equ OFF_A3,      6*REGSZ
    .equ OFF_A4,      7*REGSZ
    .equ OFF_A5,      8*REGSZ
    .equ OFF_A6,      9*REGSZ
    .equ OFF_A7,     10*REGSZ
    .equ OFF_S0,     11*REGSZ
    .equ OFF_S1,     12*REGSZ
    .equ OFF_S2,     13*REGSZ
    .equ OFF_S3,     14*REGSZ
    .equ OFF_S4,     15*REGSZ
    .equ OFF_S5,     16*REGSZ
    .equ OFF_S6,     17*REGSZ
    .equ OFF_S7,     18*REGSZ
    .equ OFF_S8,     19*REGSZ
    .equ OFF_S9,     20*REGSZ
    .equ OFF_S10,    21*REGSZ
    .equ OFF_S11,    22*REGSZ
    .equ OFF_T0,     23*REGSZ
    .equ OFF_T1,     24*REGSZ
    .equ OFF_T2,     25*REGSZ
    .equ OFF_T3,     26*REGSZ
    .equ OFF_T4,     27*REGSZ
    .equ OFF_T5,     28*REGSZ
    .equ OFF_T6,     29*REGSZ
    .equ CTX_BYTES,  30*REGSZ

.globl ctxsw
ctxsw:
    # a0 = new thread*, a1 = old thread*
    addi sp, sp, -CTX_BYTES          # Reserve space before saving
    sd   ra,      OFF_RA(sp)
    csrr t0, sstatus
    sd   t0,      OFF_SSTATUS(sp)    # save sstatus
    csrr t0, sepc
    sd   t0,      OFF_SEPC(sp)       # save sepc

    sd   a0,      OFF_A0(sp)
    sd   a1,      OFF_A1(sp)
    sd   a2,      OFF_A2(sp)
    sd   a3,      OFF_A3(sp)
    sd   a4,      OFF_A4(sp)
    sd   a5,      OFF_A5(sp)
    sd   a6,      OFF_A6(sp)
    sd   a7,      OFF_A7(sp)

    sd   s0,      OFF_S0(sp)
    sd   s1,      OFF_S1(sp)
    sd   s2,      OFF_S2(sp)
    sd   s3,      OFF_S3(sp)
    sd   s4,      OFF_S4(sp)
    sd   s5,      OFF_S5(sp)
    sd   s6,      OFF_S6(sp)
    sd   s7,      OFF_S7(sp)
    sd   s8,      OFF_S8(sp)
    sd   s9,      OFF_S9(sp)
    sd   s10,     OFF_S10(sp)
    sd   s11,     OFF_S11(sp)

    sd   t0,      OFF_T0(sp)         # t0 currently holds last sepc; harmless to save
    sd   t1,      OFF_T1(sp)
    sd   t2,      OFF_T2(sp)
    sd   t3,      OFF_T3(sp)
    sd   t4,      OFF_T4(sp)
    sd   t5,      OFF_T5(sp)
    sd   t6,      OFF_T6(sp)

    sd   sp, THREAD_STACKPTR(a1)     # Store pushed SP to old->stackptr

    # Load new address space (ASID + root PPN) into satp
    lhu  t1, THREAD_ASID(a0)         
    ld   t2, THREAD_ROOTPPN(a0)      
    slli t1, t1, 44
    or   t0, t2, t1
    li   t3, SATP_MODE_SV39
    or   t0, t0, t3
    csrw satp, t0
    sfence.vma x0, x0

    # Switch to new stack and restore context
    ld   sp, THREAD_STACKPTR(a0)

    ld   ra,      OFF_RA(sp)
    ld   t0,      OFF_SSTATUS(sp)
    csrw sstatus, t0                 # Restore full sstatus
    li   t3, (1 << 18)               # ensure SUM is set (S-mode may touch U pages)
    csrs sstatus, t3                 # Reassert SUM every switch

    ld   t0,      OFF_SEPC(sp)
    csrw sepc, t0

    ld   a0,      OFF_A0(sp)
    ld   a1,      OFF_A1(sp)
    ld   a2,      OFF_A2(sp)
    ld   a3,      OFF_A3(sp)
    ld   a4,      OFF_A4(sp)
    ld   a5,      OFF_A5(sp)
    ld   a6,      OFF_A6(sp)
    ld   a7,      OFF_A7(sp)

    ld   s0,      OFF_S0(sp)
    ld   s1,      OFF_S1(sp)
    ld   s2,      OFF_S2(sp)
    ld   s3,      OFF_S3(sp)
    ld   s4,      OFF_S4(sp)
    ld   s5,      OFF_S5(sp)
    ld   s6,      OFF_S6(sp)
    ld   s7,      OFF_S7(sp)
    ld   s8,      OFF_S8(sp)
    ld   s9,      OFF_S9(sp)
    ld   s10,     OFF_S10(sp)
    ld   s11,     OFF_S11(sp)

    ld   t0,      OFF_T0(sp)
    ld   t1,      OFF_T1(sp)
    ld   t2,      OFF_T2(sp)
    ld   t3,      OFF_T3(sp)
    ld   t4,      OFF_T4(sp)
    ld   t5,      OFF_T5(sp)
    ld   t6,      OFF_T6(sp)

    addi sp, sp, CTX_BYTES
    ret

#  `ctxload` loads a thread  onto the CPU without a  source thread.  This
#  is used during initialization to load the FIRST thread and switch from
#  bootstrap into the OS's steady state.
.globl ctxload
ctxload:
    ld sp, THREAD_STACKPTR(a0)  #-- Load stack pointer
    ld t0, THREAD_ROOTPPN(a0)   # | Load root ppn
    lhu t1, THREAD_ASID(a0)     # | Load asid (zero-extend)
    slli t1, t1, 44             # | place ASID in bits 59:44
    li t2, SATP_MODE_SV39       # | 
    or t0, t0, t1               # | add ASID
    or t0, t0, t2               # | add mode
    csrw satp, t0               # |
    sfence.vma x0, x0           # |
	.extern MMU_ENABLED         # |
    la t4, MMU_ENABLED          # |
    li t5, 1                    # |
    sb t5, 0(t4)                # |
    fence rw, rw                # |
    li t3, (1 << 18)            # | Set sstatus.SUM (bit 18) so S-mode can access U pages
    csrs sstatus, t3            # |
    ld t0, -3*REGSZ(sp)         # | Load wrapper entry point
    ld a0, -2*REGSZ(sp)         # | Load thread entry function pointer
    ld ra, -1*REGSZ(sp)         # | Load return address trampoline
    addi sp, sp, CTX_BYTES      # | Move SP above saved context frame
    jr t0                       # | Jump to wrapper         
    ret                         # -