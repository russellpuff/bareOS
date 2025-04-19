#include <barelib.h>
#include <queue.h>
#include <thread.h>
#include <syscall.h>

queue_t sleep_list;

/*  Places the thread into a sleep state and inserts it into the  *
 *  sleep delta list.                                             */
int32 sleep_thread(uint32 threadid, uint32 delay) {
	if(threadid >= NTHREADS || thread_table[threadid].state != TH_READY) return -1;
	detach_thread(threadid);
	thread_table[threadid].priority = delay; /* Save the delay for the clock to use later. */
	thread_table[threadid].state = TH_SLEEP;
	enqueue_thread(&sleep_list, threadid);
	queue_t* node = &queue_table[threadid];
	node->key = delay;
	raise_syscall(RESCHED);
	return 0;
}

/*  If the thread is in the sleep state, remove the thread from the  *
 *  sleep queue and resumes it.                                      */
int32 unsleep_thread(uint32 threadid) {
	if(threadid >= NTHREADS || !(thread_table[threadid].state & TH_SLEEP)) return -1;
	
	return 0;
}
