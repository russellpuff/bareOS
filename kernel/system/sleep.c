#include <barelib.h>
#include <queue.h>
#include <thread.h>
#include <syscall.h>

queue_t sleep_list;

/*  Places the thread into a sleep state and inserts it into the  *
 *  sleep delta list.                                             */
int32 sleep_thread(uint32 threadid, uint32 delay) {
	if(threadid >= NTHREADS || thread_table[threadid].state != TH_READY) return -1;
	detach_thread(threadid, false);
	thread_table[threadid].state = TH_SLEEP;
	queue_t* node = &queue_table[threadid];
	node->key = delay;
	delta_enqueue(&sleep_list, threadid);
	raise_syscall(RESCHED);
	return 0;
}

/*  If the thread is in the sleep state, remove the thread from the  *
 *  sleep queue and resumes it.                                      */
int32 unsleep_thread(uint32 threadid) {
	if(threadid >= NTHREADS || !(thread_table[threadid].state & TH_SLEEP)) return -1;
	detach_thread(threadid, true);
	queue_t* node = &queue_table[threadid];
	node->key = thread_table[threadid].priority;
	thread_table[threadid].state = TH_READY;
	enqueue_thread(&ready_list, threadid);
	raise_syscall(RESCHED);
	return 0;
}
