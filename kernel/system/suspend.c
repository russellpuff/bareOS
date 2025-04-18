#include <barelib.h>
#include <interrupts.h>
#include <syscall.h>
#include <thread.h>
#include <queue.h>

/*  Takes a index into the thread table of a thread to suspend.  If the thread is  *
 *  not in the  running or  ready state,  returns an error.   Otherwise, sets the  *
 *  thread's  state  to  suspended  and  raises a  RESCHED  syscall to schedule a  *
 *  different thread.  Returns the threadid to confirm suspension.                 */
int32 suspend_thread(uint32 threadid) {
  if(threadid < 0 || threadid >= NTHREADS || 
    (thread_table[threadid].state != TH_RUNNING && 
      thread_table[threadid].state != TH_READY)) {
    return -1;
  }

  /* Remove from ready_list. We can't expect it to be at the head of the list when
     this is called, so we detach it manually. TODO: Move into queue.c */
  if(thread_table[threadid].state == TH_READY) {
    queue_t* node = &queue_table[threadid];
    if(node->qnext != NULL && node->qprev != NULL) {
      (node->qprev)->qnext = node->qnext;
      (node->qnext)->qprev = node->qprev;
      node->qnext = NULL;
      node->qprev = NULL;
      node->key = -1;
    }
  }

  thread_table[threadid].state = TH_SUSPEND;
  raise_syscall(RESCHED);
  return threadid;
}
