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

	.equ CTX_BYTES, (29*REGSZ)

.globl ctxsw
ctxsw:
	sd ra,  -1*REGSZ(sp)  # --
	sd a0,  -2*REGSZ(sp)  #  |
	csrr t0, sepc         #  |
	sd t1,  -4*REGSZ(sp)  #  |
	sd t2,  -5*REGSZ(sp)  #  |
	sd s0,  -6*REGSZ(sp)  #  |
	sd s1,  -7*REGSZ(sp)  #  |
	sd a1,  -8*REGSZ(sp)  #  |
	sd a2,  -9*REGSZ(sp)  #  |
	sd a3, -10*REGSZ(sp)  #  |
	sd a4, -11*REGSZ(sp)  #  |
	sd a5, -12*REGSZ(sp)  #  |
	sd a6, -13*REGSZ(sp)  #  |
	sd a7, -14*REGSZ(sp)  #  |
	sd s2, -15*REGSZ(sp)  #  |  Store all registers onto bottom of the stack (old thread)
	sd s3, -16*REGSZ(sp)  #  |
	sd s4, -17*REGSZ(sp)  #  |
	sd s5, -18*REGSZ(sp)  #  |
	sd s6, -19*REGSZ(sp)  #  |
	sd s7, -20*REGSZ(sp)  #  |
	sd s8, -21*REGSZ(sp)  #  |
	sd s9, -22*REGSZ(sp)  #  |
	sd s10,-23*REGSZ(sp)  #  |
	sd s11,-24*REGSZ(sp)  #  |
	sd t3, -25*REGSZ(sp)  #  |
	sd t4, -26*REGSZ(sp)  #  |
	sd t5, -27*REGSZ(sp)  #  |
	sd t6, -28*REGSZ(sp)  #  |
	sd t0, -29*REGSZ(sp)  #  |
	sd t0, -3*REGSZ(sp)   # --  
	sd sp, THREAD_STACKPTR(a1)  # --  Store the current stack pointer to old thread -> stackptr

	lhu t1, THREAD_ASID(a0)     # -- Load new thread ASID
	ld t2, THREAD_ROOTPPN(a0)   # -- Load new thread root ppn
	slli t1, t1, 44             # -- Shift ASID left into place
	or t0, t1, t2               # -- Combine ASID and ppn
	li t3, SATP_MODE_SV39       # -- Get mode (8 for Sv39, will never change, pre-shifted)
	or t0, t0, t3               # -- Combine all
	csrw satp, t0               # -- Write to satp register
	sfence.vma x0, x0           # -- Flush TLB entries
	
	ld sp, THREAD_STACKPTR(a0)  # --  Load the new stack pointer from the thread table  (argument 0)
	ld ra,  -1*REGSZ(sp)  # --
	ld a0,  -2*REGSZ(sp)  #  |
	ld t0,  -3*REGSZ(sp)  #  |
	csrw sepc, t0         #  |
	ld t1,  -4*REGSZ(sp)  #  |
	ld t2,  -5*REGSZ(sp)  #  |
	ld s0,  -6*REGSZ(sp)  #  |
	ld s1,  -7*REGSZ(sp)  #  |
	ld a1,  -8*REGSZ(sp)  #  |
	ld a2,  -9*REGSZ(sp)  #  |
	ld a3, -10*REGSZ(sp)  #  |
	ld a4, -11*REGSZ(sp)  #  |
	ld a5, -12*REGSZ(sp)  #  |
	ld a6, -13*REGSZ(sp)  #  |
	ld a7, -14*REGSZ(sp)  #  |  Restore the registers from the bottom of the stack (new thread)
	ld s2, -15*REGSZ(sp)  #  |
	ld s3, -16*REGSZ(sp)  #  |
	ld s4, -17*REGSZ(sp)  #  |
	ld s5, -18*REGSZ(sp)  #  |
	ld s6, -19*REGSZ(sp)  #  |
	ld s7, -20*REGSZ(sp)  #  |
	ld s8, -21*REGSZ(sp)  #  |
	ld s9, -22*REGSZ(sp)  #  |
	ld s10,-23*REGSZ(sp)  #  |
	ld s11,-24*REGSZ(sp)  #  |
	ld t3, -25*REGSZ(sp)  #  |
	ld t4, -26*REGSZ(sp)  #  |
	ld t5, -27*REGSZ(sp)  #  |
	ld t6, -28*REGSZ(sp)  #  |
	ld t0, -29*REGSZ(sp)  # --
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
    la t4, MMU_ENABLED          # | set as true asap
    li t3, (1 << 18)            # | Set sstatus.SUM (bit 18) so S-mode can access U pages
    csrs sstatus, t3            # |
    ld a0, -2*REGSZ(sp)         # |
    ld ra, -3*REGSZ(sp)         #--
    #li t0, 0x1 # I dunno what these were for so I removed them
    #sw t0, 0(t1)                
    ret