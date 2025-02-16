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
        ctxsw(&thread_table[i].stackptr, &thread_table[current_thread].stackptr);
        return;
    }
  }
  return 0;
}
