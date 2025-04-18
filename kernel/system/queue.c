#include <thread.h>
#include <queue.h>

/*  Queues entries in bareOS are contained in the 'queue_table' array.  Each queue has a "root"
 *  that contains  pointers to  the first  and last  elements in that respective queue.  These
 *  roots are created separately - for example, the 'ready_list', which represents all threads
 *  currently available for scheduling.  Non-root nodes are stored in the 'queue_table' proper.
 *  Each node points to the previous and next node.  */

queue_t queue_table[NTHREADS];   /*  Array of queue elements          */
queue_t ready_list;              /*  Struct with the ready_list root  */

/* 'init_queues' sets all entries in the queue_table to initial values so they   *
 *  can be used safely later during OS operations.                               */
void init_queues(void) {
  
  return;
}


/*  'thread_enqueue' takes a  pointer to the "root" of a queue  and a threadid of a thread  *
 *  to add to the queue.   The thread  will be placed in the queue so that the `key` field  * 
 *  of each  queue entry is in ascending order from head to tail  and FIFO order when keys  *
 *  match.                                                                                  */
int32 enqueue_thread(queue_t* queue, uint32 threadid) {

  return 0;
}


/*  'thread_dequeue' takes a queue pointer associated with a queue "root" and removes the  *
 *  thread at the head of the queue and returns its the thread table index, ensuring that  *
 *  the queue  maintains its  structure and the head correctly points to the  next thread  *
 *  (if any).                                                                              */
int32 dequeue_thread(queue_t* queue) {

  return 0;
}
