#include <lib/barelib.h>
#include <lib/bareio.h>
#include <system/thread.h>
#include <system/interrupts.h>
#include <system/syscall.h>
#include <device/timer.h>
#include <mm/vm.h>

volatile unsigned long s_last_scause, s_last_stval, s_last_sepc;

/*
 *  This file contains code for handling exceptions generated
 *  by the hardware   (see '__traps' in bootstrap.s)
 *
 *  We can add a new syscall by writing a function and adding
 *  it to the 'syscall_table' below.
 */

void (*syscall_table[]) (void) = {
  resched
};

s_interrupt handle_syscall(void) {
  int32_t table_size = sizeof(syscall_table) / sizeof(int (*)(void));
  if (signum < table_size)
    syscall_table[signum]();
}

m_interrupt delegate_clk(void) {
    *(uint64_t*)CLINT_TIMER_ADDR += timer_interval;
    asm volatile ("li t0, 0x20\n"
		  "csrs mip, t0\n"
		  "csrs mie, t0\n");
}

/* Rudimentary exception handler, will handle more exceptions as time goes on. */
__attribute__((noinline, optimize("O0")))
m_interrupt handle_exception(void) {
    uint64_t cause, tval, epc;
    asm volatile("csrr %0, scause" : "=r"(cause));
    asm volatile("csrr %0, stval"  : "=r"(tval));
    asm volatile("csrr %0, sepc"   : "=r"(epc));
    s_last_scause = cause;
    s_last_stval = tval;
    s_last_sepc = epc;

    if ((cause & (1ULL << 63)) == 0) { /* Synchronous exception */
        uint64_t code = cause & 0xfffULL; /* Get exception code */
        /* Placeholder for handling basic page faults without a crash */
        /* 12 = instruction page fault */
        /* 13 = load page fault        */
        /* 15 = store page fault */
        if (code == 12 || code == 13 || code == 15) {
            /* No handler. Just kill the thread. */
            kprintf("Thread %u faulted at 0x%x on code %u\n", current_thread, (uint32_t)tval, code);
            kill_thread(current_thread);
            raise_syscall(RESCHED);
            return;
        }
    }

    kprintf("Unhandled exception: scause=0x%lu stval=0x%lu\n", cause, tval);
    while (1);
}

