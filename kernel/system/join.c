#include <barelib.h>
#include <thread.h>
#include <interrupts.h>
#include <syscall.h>


/*  Takes an index into the thread_table.  If the thread is TH_DEFUNCT,  *
 *  mark  the thread  as TH_FREE  and return its  `retval`.   Otherwise  *
 *  raise RESCHED and loop to check again later.                         */
int32 join_thread(uint32 threadid) {

  return 0;
}
