#include <barelib.h>
#include <thread.h>

/*  'resched' places the current running thread into the ready state  *
 *  and  places it onto  the tail of the  ready queue.  Then it gets  *
 *  the head  of the ready  queue  and sets this  new thread  as the  *
 *  'current_thread'.  Finally,  'resched' uses 'ctxsw' to swap from  *
 *  the old thread to the new thread.                                 */
void resched(void) {
  for(uint32 i = current_thread + 1; i != current_thread; i = (i + 1) % NTHREADS) {
    if(thread_table[i].state == TH_READY) {
      uint32 old_thread = current_thread;
      current_thread = i;
      thread_table[i].state = TH_RUNNING;
      if(thread_table[old_thread].state & TH_RUNNABLE) { thread_table[old_thread].state = TH_READY; }
      ctxsw(&(thread_table[i].stackptr), &(thread_table[old_thread].stackptr));
      return;
    }
  }
}
