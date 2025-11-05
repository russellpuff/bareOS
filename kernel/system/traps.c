#include <lib/barelib.h>
#include <lib/bareio.h>
#include <system/thread.h>
#include <system/exec.h>
#include <system/interrupts.h>
#include <system/syscall.h>
#include <system/queue.h>
#include <lib/ecall.h>
#include <system/panic.h>
#include <device/timer.h>
#include <mm/vm.h>
#include <mm/malloc.h>

/*
 *  This file contains code for handling exceptions generated
 *  by the hardware   (see '__traps' in bootstrap.s)
 *
 */

void s_handle_exception(uint64_t* frame) {
   uint64_t cause, tval, epc;
   asm volatile("csrr %0, scause" : "=r"(cause));
   asm volatile("csrr %0, stval"  : "=r"(tval));
   asm volatile("csrr %0, sepc"   : "=r"(epc));

	if ((cause & (1ULL << 63)) == 0) { /* Synchronous exception */
		uint64_t code = cause & 0xfffULL; /* Get exception code */
		/* Placeholder for handling basic page faults without a crash */
		/* 12 = instruction page fault */
		/* 13 = load page fault        */
		/* 15 = store page fault */
		if (code == 12 || code == 13 || code == 15) {
			/* No handler. Just kill the thread. */
			krprintf("Thread %u faulted at %x on code %u\n", current_thread, (uint32_t)tval, code);
			if (ready_list.qnext == &ready_list) {
				panic("Couldn't resched after a fault. There's no other available threads to switch to.\n");
			}
			kill_thread(current_thread);
			pend_resched(RESCHED);
			return;
		}
	}

	panic("Supervisor exception: scause=%x stval=%x, sepc=%x\nFatal. The system has halted.\n", 
		cause, tval, epc);
}

void m_handle_exception(void) {
	uint64_t cause, tval, epc, mstatus;

	asm volatile("csrr %0, mcause" : "=r"(cause));
	asm volatile("csrr %0, mtval"  : "=r"(tval));
	asm volatile("csrr %0, mepc"   : "=r"(epc));
	asm volatile("csrr %0, mstatus" : "=r"(mstatus));

	panic("Machine exception: mcause=%x mtval=%x mepc=%x mstatus=%x\nFatal. The system has halted.\n", 
		cause, tval, epc, mstatus);
}