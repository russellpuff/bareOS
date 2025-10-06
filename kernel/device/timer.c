#include <lib/barelib.h>
#include <system/interrupts.h>
#include <system/syscall.h>
#include <system/queue.h>
#include <system/thread.h>
#include <device/timer.h>

/*
 *  This file contains functions for initializing and handling interrupts
 *  from the hardware timer.
 */

#define TRAP_TIMER_ENABLE 0xa0

volatile uint64_t* clint_timer_addr  = (uint64_t*)0x2004000;
const uint64_t timer_interval = 100000;

/*
 * This function is called as part of the bootstrapping sequence
 * to enable the timer. (see bootstrap.s)
 */
void init_clk(void) {
  *clint_timer_addr = timer_interval;
  set_interrupt(TRAP_TIMER_ENABLE);
}

/* 
 * This function is triggered every 'timer_interval' 
 * automatically.  (see '__traps' in bootstrap.s)
 */
s_interrupt handle_clk(void) {
  acknowledge_interrupt();
  sleep_list.qnext->key -= 10;
  while(sleep_list.qnext->key == 0) {
	uint32_t tid = dequeue_thread(&sleep_list);
	if(tid == -1) continue;
	unsleep_thread(tid);
  }
  raise_syscall(RESCHED);
}
