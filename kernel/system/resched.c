#include <barelib.h>
#include <thread.h>
#include <queue.h>

/*  'resched' places the current running thread into the ready state  *
 *  and  places it onto  the tail of the  ready queue.  Then it gets  *
 *  the head  of the ready  queue  and sets this  new thread  as the  *
 *  'current_thread'.  Finally,  'resched' uses 'ctxsw' to swap from  *
 *  the old thread to the new thread.                                 */
void resched(void) {
	uint32 new_thread = dequeue_thread(&ready_list);
	if(new_thread == -1) return;
	uint32 old_thread = current_thread;
	current_thread = new_thread;
	thread_table[new_thread].state = TH_RUNNING;
	if(thread_table[old_thread].state & TH_READY) 
	{ 
		thread_table[old_thread].state = TH_READY; 
		queue_table[old_thread].key = thread_table[old_thread].priority;
		enqueue_thread(&ready_list, old_thread);
	}
	ctxsw(&(thread_table[new_thread].stackptr), &(thread_table[old_thread].stackptr));
	return;
}
