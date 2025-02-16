#include <barelib.h>
#include <thread.h>

/*
 *  This file contains code for handling exceptions generated
 *  by the hardware   (see '__traps' in bootstrap.s)
 *
 *  We can add a new syscall by writing a function and adding
 *  it to the 'syscall_table' below.
 */

void (*syscall_table[]) (void) = {
  [0] = resched
};

extern int32 signum;
s_interrupt handle_syscall(void) {
  int32 table_size = sizeof(syscall_table) / sizeof(int (*)(void));
  if (signum < table_size)
    syscall_table[signum]();
}

extern volatile uint32* clint_timer_addr;
extern const uint32 timer_interval;

m_interrupt delegate_clk(void) {
    *clint_timer_addr += timer_interval;
    asm volatile ("li t0, 0x20\n"
		  "csrs mip, t0\n"
		  "csrs mie, t0\n");
}

/*
 *  This function is a placeholder for implementing handler behavior for
 *  exceptions (such as Load Access Fault or Illegal Instruction exceptions)
 *  Initially, bareOS ignores any non-ECALL exceptions, and the behavior
 *  of attempting an instruction that triggers such a fault is undefined.
 */
m_interrupt handle_exception(void) {
  
}

