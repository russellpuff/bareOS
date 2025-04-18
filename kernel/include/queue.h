#ifndef H_QUEUE
#define H_QUEUE

#include <barelib.h>

/*  Certain  OS  features  require  threads  to  be  queued.  *
 *  Because each  thread can  only belong  to one queue at a  *
 *  time, all  queues are  stored  in a single  thread_queue  *
 *  list (see system/queue.c) which uses indices to point to  *
 *  the next and previous element in a given queue.           */
typedef struct _queue {
  int32 key;             /*  An arbitrary key value for the thread, meaning depends on which queue it is in  */
  struct _queue* qprev;  /*  The next element in the queue                                                   */
  struct _queue* qnext;  /*  The previous element in the queue                                               */
} queue_t;

extern queue_t queue_table[];
extern queue_t ready_list;

/*  thread related prototypes  */
int32 enqueue_thread(queue_t*, uint32);
int32 dequeue_thread(queue_t*);
void init_queues(void);

#endif
