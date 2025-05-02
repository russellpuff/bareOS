#include <barelib.h>
#include <thread.h>
#include <interrupts.h>
#include <syscall.h>
#include <semaphore.h>

/*  Takes an index into the thread_table.  If that thread is not free (in use),  *
 *  sets the thread to defunct and raises a RESCHED syscall.                     */
int32 kill_thread(uint32 threadid) {
  if (threadid >= NTHREADS || thread_table[threadid].state == TH_FREE) /*                                                             */
    return -1;                                                         /*  Return if the requested thread is invalid or already free  */

  for (int i=0; i<NTHREADS; i++) {          /*                                        */
    if (thread_table[i].parent == threadid) /*  Identify all children of the thread   */
      thread_table[i].state = TH_FREE;      /*  Reap running children threads         */
  }
  thread_table[threadid].state = TH_DEFUNCT; /*  Set the thread's state to TH_DEFUNCT  */
  free_sem(&thread_table[threadid].sem); /* Calls resched after dumping children. */
  return 0;
}
