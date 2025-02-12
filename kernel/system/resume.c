#include <barelib.h>
#include <interrupts.h>
#include <syscall.h>

/*  Takes a index into the thread table of a thread to resume.  If the thread is already  *
 *  ready  or running,  returns an error.  Otherwise, adds the thread to the ready list,  *
 *  sets  the thread's  state to  ready and raises a RESCHED  syscall to  schedule a new  *
 *  thread.  Returns the threadid to confirm resumption.                                  */
int32 resume_thread(uint32 threadid) {
  if(threadid < 0 || threadid >= NTHREADS || 
    thread_table[threadid].state == TH_RUNNING && 
      thread_table[threadid].state == TH_READY) {
    return -1;
  }
  thread_table[threadid].state = TH_READY;
  raise_syscall(RESCHED);
  return threadid;
}

