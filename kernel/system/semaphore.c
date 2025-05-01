#include <interrupts.h>
#include <queue.h>
#include <semaphore.h>

/*
 *  All Semaphore operations should use a mutex to prevent another thread
 *  from modifying the  semaphore and corrupt the  struct due to a poorly 
 *  timed clock tick.
 */

/*  Creates a semaphore_t structure and  initializes it to base  *
 *  values.  The count is assigned to the semaphore's key value  */
semaphore_t create_sem(int32 count) {
	semaphore_t sem;
	sem.state = S_USED;
	sem.queue.key = count;
	sem.queue.qnext = sem.queue.qprev = &sem.queue;
	return sem;
}

/*  Marks a semaphore as free and release all waiting threads  */
int32 free_sem(semaphore_t* sem) {

  
  return 0;
}

/*  Decrements the given  semaphore if it is in use.  If  the semaphore  *
 *  is less than 0, marks the thread as waiting and switches to another  *
 *  another thread.                                                      */
int32 wait_sem(semaphore_t* sem) {


  return 0;
}

/*  Increments the given semaphore if it is in use.  Resume the next  *
 *  waiting thread (if any).                                          */
int32 post_sem(semaphore_t* sem) {

  
  return 0;
}
