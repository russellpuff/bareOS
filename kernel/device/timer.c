#include <lib/barelib.h>
#include <system/interrupts.h>
#include <system/syscall.h>
#include <system/queue.h>
#include <system/thread.h>
#include <system/panic.h>
#include <device/timer.h>
#include <mm/vm.h>

/*
 *  This file contains functions for initializing and handling interrupts
 *  from the hardware timer.
 */

#define TRAP_TIMER_ENABLE 0xa0
#define CLINT_MTIME 0x0200BFF8
const uint64_t timer_interval = 100000;
const uint64_t clint_timer_addr = 0x2004000;

/*
 * This function is called as part of the bootstrapping sequence
 * to enable the timer. (see bootstrap.s)
 */
void init_clk(void) {
	volatile uint64_t* mtime = (uint64_t*)CLINT_MTIME;
	volatile uint64_t* mtimecmp = (uint64_t*)clint_timer_addr;
	*mtimecmp = *mtime + timer_interval;
	set_m_interrupt(TRAP_TIMER_ENABLE);
}

void handle_clk(void) {
	if (sleep_list.qnext != &sleep_list) {
		if (sleep_list.qnext->key == 0) {
			panic("The next thread in the sleep list had a timer of zero but was not dequeued.\n");
		}
		sleep_list.qnext->key -= 10;
		if (sleep_list.qnext->key < 0) {
			panic("Sleep-list invariant violated: next->key (%d) not a non-negative multiple of 10.\n", sleep_list.qnext->key);
		}
		while (sleep_list.qnext->key == 0) {
			uint32_t tid = dequeue_thread(&sleep_list);
			if (tid == -1) continue; /* Theoretically impossible. Ignoring the phantom thread is better than panicking, for now. */
			unsleep_thread(tid);
		}
	}
	
	raise_syscall(RESCHED);
}
