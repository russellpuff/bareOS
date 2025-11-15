/*
 *    This file contains the code run by the system when it boots.  The .text.entry
 *    function '_start' is placed in memory where the program counter is initialized
 *        (see kernel.ld)
 *    It sets up the necessary registers then calls the 'initialize' C function.
 */

	.file "bootstrap.s"
	.equ _mstatus_init,       0x880

.section .bss
.align 7
.globl m_trap_stack
m_trap_stack:
		.skip 1024
.globl m_trap_stack_top
m_trap_stack_top:

.section .text.entry
.globl _start
_start:
	csrr t0, mhartid             # -.
	bne t0, x0, idle             # -'    Skip setup on all harts execpt hart 0

	la t1, _mstatus_init         # --
	or t0, t0, t1                #  |    Enable interrupts and set running state to Supervisor mode
	csrw mstatus, t0             # --
	li t0, 0x202
	csrs mideleg, t0

	la t0, __m_trap_vector       # --
	addi t0, t0, 0x1             #  |    Set exception and interrupt vector to the '__traps' label
	csrw mtvec, t0               # --

	li t0, 0xB300			     # -.
	csrs medeleg, t0			 # -'    Delegate page faults and ecall to supervisor

	la gp, _kmap_global_ptr      # --
	la sp, mem_start             #  |    Set initial stack pointer, global pointer,
	la t0, m_trap_stack_top      #  |    Provide stack space for machine-mode traps
	csrw mscratch, t0            #  |    and system entry function
	la t0, supervisor_start      #  |    
	csrw mepc, t0                # --

	li t0, 0x0f0f                # --
	li t1, 0x20000000            #  |
	li t2, 0x22000000            #  |    Set up memory protection so that Supervisor mode
	csrw pmpcfg0, t0             #  |    can acccess all regions of memory
	csrw pmpaddr0, t1            #  |
	csrw pmpaddr1, t2            # --

	csrsi mie, 0x2
	call init_clk                # --    Initialize clock interrupts
	call init_plic	             # --    Initialize external interrupts

	la ra, idle                  # -.    Set the return point for the kernel to idle
	mret                         # -'    Return to Supervisor mode at 'initialize'

idle:                            # --
	wfi                          #  | Loop forever if not hart0
	j idle                       # --
