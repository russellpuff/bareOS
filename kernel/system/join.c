#include <barelib.h>
#include <thread.h>
#include <interrupts.h>
#include <syscall.h>


/*  Takes an index into the thread_table.  If the thread is TH_DEFUNCT,  *
 *  mark  the thread  as TH_FREE  and return its  `retval`.   Otherwise  *
 *  raise RESCHED and loop to check again later.                         */
int32 join_thread(uint32 threadid) {
  /* TODO: Check if there's invalid states for this. */
  if(threadid < 0 || threadid >= NTHREADS || thread_table[threadid].state == TH_FREE) {
    return -1;
  }

  while(thread_table[threadid].state != TH_DEFUNCT) {
    raise_syscall(RESCHED);
  }
  thread_table[threadid].state = TH_FREE;
  return thread_table[threadid].retval;
}
