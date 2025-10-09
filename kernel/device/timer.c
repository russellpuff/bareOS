#include <lib/barelib.h>
#include <system/interrupts.h>
#include <system/syscall.h>
#include <system/queue.h>
#include <system/thread.h>
#include <device/timer.h>
#include <mm/vm.h>

/*
 *  This file contains functions for initializing and handling interrupts
 *  from the hardware timer.
 */

#define TRAP_TIMER_ENABLE 0xa0
#define STIP_CLEAR (1UL << 5)
const uint64_t timer_interval = 100000;

/*
 * This function is called as part of the bootstrapping sequence
 * to enable the timer. (see bootstrap.s)
 */
void init_clk(void) {
  *(uint64_t*)CLINT_TIMER_ADDR = timer_interval;
  set_interrupt(TRAP_TIMER_ENABLE);
}
