	.section .text
	.align 2

#  context layout (must match thread.h) 
	.equ CTX_RA,   0
	.equ CTX_S0,   8
	.equ CTX_S1,   16
	.equ CTX_S2,   24
	.equ CTX_S3,   32
	.equ CTX_S4,   40
	.equ CTX_S5,   48
	.equ CTX_S6,   56
	.equ CTX_S7,   64
	.equ CTX_S8,   72
	.equ CTX_S9,   80
	.equ CTX_S10,  88
	.equ CTX_S11,  96
	.equ CTX_SP,   104
	.equ CTX_SIZE, 112

#  trapframe layout (must match thread.h)
	.equ TF_RA,      0
	.equ TF_SP,      8
	.equ TF_GP,      16
	.equ TF_TP,      24
	.equ TF_T0,      32
	.equ TF_T1,      40
	.equ TF_T2,      48
	.equ TF_T3,      56
	.equ TF_T4,      64
	.equ TF_T5,      72
	.equ TF_T6,      80
	.equ TF_S0,      88
	.equ TF_S1,      96
	.equ TF_S2,      104
	.equ TF_S3,      112
	.equ TF_S4,      120
	.equ TF_S5,      128
	.equ TF_S6,      136
	.equ TF_S7,      144
	.equ TF_S8,      152
	.equ TF_S9,      160
	.equ TF_S10,     168
	.equ TF_S11,     176
	.equ TF_A0,      184
	.equ TF_A1,      192
	.equ TF_A2,      200
	.equ TF_A3,      208
	.equ TF_A4,      216
	.equ TF_A5,      224
	.equ TF_A6,      232
	.equ TF_A7,      240
	.equ TF_SEPC,    248
	.equ TF_SSTATUS, 256
	.equ TF_SIZE,    264

#  void ctxsw(context *prev, context *next, uint64_t next_satp)
	.globl ctxsw
ctxsw:
	# a0=prev, a1=next, a2=next satp, a3=next kstack top
	# Save callee-saved + sp into *prev
	sd   ra,  CTX_RA(a0)
	sd   s0,  CTX_S0(a0)
	sd   s1,  CTX_S1(a0)
	sd   s2,  CTX_S2(a0)
	sd   s3,  CTX_S3(a0)
	sd   s4,  CTX_S4(a0)
	sd   s5,  CTX_S5(a0)
	sd   s6,  CTX_S6(a0)
	sd   s7,  CTX_S7(a0)
	sd   s8,  CTX_S8(a0)
	sd   s9,  CTX_S9(a0)
	sd   s10, CTX_S10(a0)
	sd   s11, CTX_S11(a0)
	sd   sp,  CTX_SP(a0)

	#  Load next's kernel context
	ld   sp,  CTX_SP(a1)
	ld   ra,  CTX_RA(a1)
	ld   s0,  CTX_S0(a1)
	ld   s1,  CTX_S1(a1)
	ld   s2,  CTX_S2(a1)
	ld   s3,  CTX_S3(a1)
	ld   s4,  CTX_S4(a1)
	ld   s5,  CTX_S5(a1)
	ld   s6,  CTX_S6(a1)
	ld   s7,  CTX_S7(a1)
	ld   s8,  CTX_S8(a1)
	ld   s9,  CTX_S9(a1)
	ld   s10, CTX_S10(a1)
	ld   s11, CTX_S11(a1)

	# Flip address space AFTER moving to next's KSTACK
	csrw satp, a2
	sfence.vma x0, x0

	addi t0, a3, -CTX_SIZE
	csrw sscratch, t0

	ret

#  void ctxload(uint64_t next_satp, uint64_t next_kstack_top)
#  Loads the first thread like ctxsw() without a prev, enables MMU
	.globl ctxload
ctxload:
	csrw satp, a0
	sfence.vma x0, x0

	.extern MMU_ENABLED
	la   t4, MMU_ENABLED
	li   t5, 1
	sb   t5, 0(t4)
	fence rw, rw
	li   t3, (1 << 18)
	csrs sstatus, t3

	addi t0, a1, -CTX_SIZE
	csrw sscratch, t0
	addi sp, t0, -TF_SIZE
	ret

# void trapret(trapframe *tf)
# Restore ALL regs from tf on current KSTACK, program sepc/sstatus, then sret.
# Note: This returns to privilege encoded in sstatus.SPP.
.globl trapret
trapret:
	ld t0, TF_SEPC(a0)
	csrw sepc, t0
	ld t1, TF_SSTATUS(a0)
	li t2, ~0x2             # clear SIE
	and t1, t1, t2  
	csrw sstatus, t1
	ld t3, TF_SP(a0)
	mv sp, t3
	ld ra, TF_RA(a0)
	ld gp, TF_GP(a0)
	ld tp, TF_TP(a0)

	ld t0, TF_T0(a0) 
	ld t1, TF_T1(a0)  
	ld t2, TF_T2(a0)
	ld t3, TF_T3(a0)  
	ld t4, TF_T4(a0)
	ld t5, TF_T5(a0)  
	ld t6, TF_T6(a0)

	ld s0, TF_S0(a0) 
	ld s1, TF_S1(a0)
	ld s2, TF_S2(a0) 
	ld s3, TF_S3(a0)
	ld s4, TF_S4(a0)  
	ld s5, TF_S5(a0) 
	ld s6, TF_S6(a0)  
	ld s7, TF_S7(a0)
	ld s8, TF_S8(a0) 
	ld s9, TF_S9(a0)
	ld s10, TF_S10(a0)
	ld s11, TF_S11(a0)

	ld   a1, TF_A1(a0)  
	ld a2, TF_A2(a0)
	ld a3, TF_A3(a0) 
	ld a4, TF_A4(a0)
	ld   a5, TF_A5(a0) 
	ld a6, TF_A6(a0) 
	ld a7, TF_A7(a0)
	ld   a0, TF_A0(a0)

	sret
