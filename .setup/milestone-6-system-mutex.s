.global create_mutex
create_mutex:                          # --
	amoswap.w.rl zero, zero, (a0)  #  |  Atomically set the value of the pointer to zero
	ret                            # --

.globl lock_mutex
lock_mutex:
	li t0, 1
1:
	lw t1, (a0)                # -.  Load the mutex value until it returns as unlocked
	bnez t1, 1b                # -'  
	amoswap.w.aq t1, t0, (a0)  # --  Try to set the mutex to 1 if it
	bnez t1, 1b                #  |  was changed before set occurs, loop
	ret                        # --  and try the whole process again

.globl release_mutex
release_mutex:
	amoswap.w.rl zero, zero, (a0)  # --  Set the mutex value to zero
	ret
