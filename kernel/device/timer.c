/*
 *  This file contains functions for initializing and handling interrupts
 *  from the hardware timer.
 */

#include <barelib.h>
#include <interrupts.h>
#include <syscall.h>
#include <queue.h>
#include <sleep.h>

#define TRAP_TIMER_ENABLE 0xa0

volatile uint32_t* clint_timer_addr  = (uint32_t*)0x2004000;
const uint32_t timer_interval = 100000;

/*
 * This function is called as part of the bootstrapping sequence
 * to enable the timer. (see bootstrap.s)
 */
void init_clk(void) {
  *clint_timer_addr = timer_interval;
  set_interrupt(TRAP_TIMER_ENABLE);
}

/* 
 * This function is triggered every 'timer_interval' microseconds 
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
