	.file "ctxsw.s"
	.option arch, +zicsr
	.equ REGSZ, 8

	.globl _sys_thread_loaded
_sys_thread_loaded:
	.word 0

#  `ctxsw`  takes two arguments, a source  thread and destination thread.  It
#  saves the current  state of the CPU into the  source thread's  table entry
#  then restores the state of the destination thread onto the CPU and returns
.globl ctxsw
ctxsw:
	sd ra,  -1*REGSZ(sp)  # --
	sd a0,  -2*REGSZ(sp)  #  |
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
	sd s6, -28*REGSZ(sp)  #  |
	sd t0, -29*REGSZ(sp)  #  |
	csrr t0, sepc         #  |
	sd t0, -3*REGSZ(sp)   # --  
	sd sp, 0(a1)          # --  Store the current stack pointer to the thread table (argument 1)
	
	ld sp, 0(a0)          # --  Load the new stack pointer from the thread table  (argument 0)
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
	ld s6, -28*REGSZ(sp)  #  |
	ld t0, -29*REGSZ(sp)  # --
	ret

#  `ctxload` loads a thread  onto the CPU without a  source thread.  This
#  is used during initialization to load the FIRST thread and switch from
#  bootstrap into the OS's steady state.
.globl ctxload
ctxload:                            # --
	ld sp, 0(a0)                #  | Set the stack pointer to a provided memory address (first argument)
	ld a0, -2*REGSZ(sp)         #  | Load the entry point procedure's address from the stack
	ld ra, -3*REGSZ(sp)         #  | Load the wrapper's address from the stack
	li t0, 0x1                  #  |
	sw t0,_sys_thread_loaded,t1 #  | Mark the thread as loaded (for validation)
	ret                         # --
