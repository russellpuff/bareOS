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

#define S_TIMER_ENABLE 0xa0
const uint64_t timer_interval = 100000;
const uint64_t clint_timer_addr = 0x2004000;

/*
 * This function is called as part of the bootstrapping sequence
 * to enable the timer. (see bootstrap.s)
 */
void init_clk(void) {
	*(uint64_t*)clint_timer_addr = timer_interval;
	set_interrupt(S_TIMER_ENABLE);
}
#include <lib/bareio.h>
s_interrupt handle_clk(void) {
	acknowledge_interrupt(0x20);
	kprintf("timer interrupt\n");
	sleep_list.qnext->key -= 10;
	while (sleep_list.qnext->key == 0) {
		uint32_t tid = dequeue_thread(&sleep_list);
		if (tid == -1) continue;
		unsleep_thread(tid);
	}
	raise_syscall(RESCHED);
}
