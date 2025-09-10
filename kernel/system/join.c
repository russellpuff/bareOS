#include <lib/barelib.h>
#include <system/thread.h>
#include <system/interrupts.h>
#include <system/syscall.h>
#include <system/semaphore.h>

/*  Takes an index into the thread_table.  If the thread is TH_DEFUNCT,  *
 *  mark  the thread  as TH_FREE  and return its  `retval`.   Otherwise  *
 *  raise RESCHED and loop to check again later.                         */
int32_t join_thread(uint32_t threadid) {
  if(threadid >= NTHREADS || thread_table[threadid].state == TH_FREE) {
    return -1;
  }

  if(thread_table[threadid].state != TH_DEFUNCT) {
	  wait_sem(&thread_table[threadid].sem);
  }
  
  thread_table[threadid].state = TH_FREE;
  return thread_table[threadid].retval;
}
