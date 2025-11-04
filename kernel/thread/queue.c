#include <system/thread.h>
#include <system/queue.h>

/*  Queues entries in bareOS are contained in the 'queue_table' array.  Each queue has a "root"
 *  that contains  pointers to  the first  and last  elements in that respective queue.  These
 *  roots are created separately - for example, the 'ready_list', which represents all threads
 *  currently available for scheduling.  Non-root nodes are stored in the 'queue_table' proper.
 *  Each node points to the previous and next node.  */

queue_t queue_table[NTHREADS];   /*  Array of queue elements          */
queue_t ready_list;              /*  Struct with the ready_list root  */
queue_t reap_list;

/* 'init_queues' sets all entries in the queue_table to initial values so they   *
 *  can be used safely later during OS operations.                               */
void init_queues(void) {
  for(uint32_t i = 0; i < NTHREADS; ++i) {
	queue_table[i].key = -1;
	queue_table[i].qnext = NULL;
	queue_table[i].qprev = NULL;
  }
  ready_list.key = sleep_list.key = reap_list.key = 0;
  ready_list.qnext = ready_list.qprev = &ready_list;
  sleep_list.qnext = sleep_list.qprev = &sleep_list;
  reap_list.qnext = reap_list.qprev = &reap_list;

  return;
}

/*  'enqueue_thread' takes a  pointer to the "root" of a queue  and a threadid of a thread  *
 *  to add to the queue. The thread  will be placed in the queue so that the `key` field  * 
 *  of each  queue entry is in ascending order from head to tail  and FIFO order when keys  *
 *  match. */

/* These are wrapper functions because the test uses normal enqueue the "correct" way. */
int32_t enqueue_thread(queue_t* queue, uint32_t threadid) {
	return enqueue(queue, threadid, false);
}

int32_t delta_enqueue(queue_t* queue, uint32_t threadid) {
	return enqueue(queue, threadid, true);
}

/* the real enqueue thread function */
int32_t enqueue(queue_t* queue, uint32_t threadid, bool delta) {
	if(threadid >= NTHREADS) return -1;
	queue_t* node = &queue_table[threadid];
	if(node->qprev != NULL || node->qnext != NULL) return -1; /* In a queue already. */
	if(!delta) {
		node->key = thread_table[threadid].priority;
	}

	/* FIFO on same key in ascending order means same key is closer to tail, so...
	   Loop until we're past the point where the node needs to be entered and shove
	   it before that point. */
	queue_t* curr = queue->qnext;
	if(delta) { /* In delta mode, we sort by cumulative rather than absolute. */
		uint32_t delay = node->key;
			while (curr != queue && delay >= curr->key) {
				delay -= curr->key;
				curr = curr->qnext;
			}
		node->key = delay;
	} else {
		while(curr != queue && curr->key <= node->key)
			curr = curr->qnext;
	}

	(curr->qprev)->qnext = node;
	node->qprev = curr->qprev;
	curr->qprev = node;
	node->qnext = curr;

	if (delta && curr != queue)
    	curr->key -= node->key;
  	return 0;
}

/*  'dequeue_thread' takes a queue pointer associated with a queue "root" and removes the  *
 *  thread at the head of the queue and returns its the thread table index, ensuring that  *
 *  the queue maintains its structure and the head correctly points to the next thread  *
 *  (if any).                                                                              */
int32_t dequeue_thread(queue_t* queue) {
	if(queue->qprev == NULL || queue->qnext == NULL || 
		queue->qnext == queue || queue->qprev == queue) 
		return -1;
	queue_t* node = queue->qnext;
	
	uint32_t threadid = 0;
	for(; threadid < NTHREADS; ++threadid)
		if(&queue_table[threadid] == node) break;
	if(threadid == NTHREADS) return -1;

	/* Not sure how else to implement. Detach and unsleep are mutually exclusive. */
	if(queue == &sleep_list) {
		(node->qnext)->key += node->key;
	}

	(node->qprev)->qnext = node->qnext;
	(node->qnext)->qprev = node->qprev;
	node->qprev = node->qnext = NULL;
	return threadid;
}

/* 'detach_thread' removes a thread from a queue regardless of where in the queue
    that thread actually is. Will fail if the thread isn't in a queue. */
int32_t detach_thread(uint32_t threadid, bool delta) {
	if(threadid >= NTHREADS) return -1;
	queue_t* node = &queue_table[threadid];
	if(node->qprev == NULL || node->qnext == NULL) return -1;

	if(delta & (node->qnext != &sleep_list)) { /* In delta mode, we need to adjust the relative key of the next node first. */
		(node->qnext)->key += node->key;
	}

	(node->qprev)->qnext = node->qnext;
	(node->qnext)->qprev = node->qprev;
	node->qnext = NULL;
	node->qprev = NULL;
	node->key = -1;
	return 0;
}
