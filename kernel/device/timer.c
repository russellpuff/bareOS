/*
 *  This file contains functions for initializing and handling interrupts
 *  from the hardware timer.
 */

#include <barelib.h>
#include <interrupts.h>

#define TRAP_TIMER_ENABLE 0xa0

volatile uint32* clint_timer_addr  = (uint32*)0x2004000;
const uint32 timer_interval = 100000;

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
  /* Student timer code goes here */
  /* ---------------------------- */
  
  
  
  /* ---------------------------- */
}
