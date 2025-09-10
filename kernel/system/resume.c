#include <lib/barelib.h>
#include <system/interrupts.h>
#include <system/syscall.h>
#include <system/thread.h>
#include <system/queue.h>
#include <system/semaphore.h>

/*  Takes a index into the thread table of a thread to resume.  If the thread is already  *
 *  ready  or running,  returns an error.  Otherwise, adds the thread to the ready list,  *
 *  sets  the thread's  state to  ready and raises a RESCHED  syscall to  schedule a new  *
 *  thread.  Returns the threadid to confirm resumption.                                  */
int32_t resume_thread(uint32_t threadid) {
  if(threadid >= NTHREADS || (thread_table[threadid].state != TH_SUSPEND && thread_table[threadid].state != TH_WAITING))
    return -1;
    
  thread_table[threadid].state = TH_READY;
  enqueue_thread(&ready_list, threadid);
  raise_syscall(RESCHED);
  return threadid;
}
