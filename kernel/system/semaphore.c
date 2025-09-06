#include <interrupts.h>
#include <queue.h>
#include <semaphore.h>
#include <syscall.h>

static byte MUTEX_LOCK;

/* This function is to avoid a possible pitfall involving
   threads with their own priorities being ordered wrongly
   in the semaphore queue. Currently assuming raw FIFO.   */
int32 sem_enqueue(queue_t* queue, uint32 threadid) {
	/* Init queue. */
	if(queue->qnext == NULL) {
		queue->qnext = queue->qprev = queue;
	}
	queue_t* node = &queue_table[threadid];
	queue_t* tail = queue->qprev;
	tail->qnext = node;
	node->qprev = tail;
	node->qnext = queue;
	queue->qprev= node;
	return 0;
}

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
	sem.queue.qnext = sem.queue.qprev = NULL;
	return sem;
}

/*  Marks a semaphore as free and release all waiting threads  */
int32 free_sem(semaphore_t* sem) {
    lock_mutex(&MUTEX_LOCK);
    if(sem->state == S_FREE) {
        release_mutex(&MUTEX_LOCK);
        return -1;
    }
	if(sem->queue.qnext != NULL) {
		while(sem->queue.qnext != &sem->queue) {
			int32 threadid = dequeue_thread(&sem->queue);
			if(threadid >= 0) {
				resume_thread(threadid);
	        }
	    }
    }
    sem->state = S_FREE;
    release_mutex(&MUTEX_LOCK);
    raise_syscall(RESCHED);
    return 0;
}

/*  Decrements the given  semaphore if it is in use.  If  the semaphore  *
 *  is less than 0, marks the thread as waiting and switches to another  *
 *  another thread.                                                      */
int32 wait_sem(semaphore_t* sem) {
	lock_mutex(&MUTEX_LOCK);
	if(sem->state == S_FREE) {
		release_mutex(&MUTEX_LOCK);
		return -1;
	}
	--sem->queue.key;
	if(sem->queue.key >= 0) {
		release_mutex(&MUTEX_LOCK);
		return 0;
	}
	thread_table[current_thread].state = TH_WAITING;
	sem_enqueue(&sem->queue, current_thread);
	release_mutex(&MUTEX_LOCK);
	raise_syscall(RESCHED);
 	return 0;
}

/*  Increments the given semaphore if it is in use.  Resume the next  *
 *  waiting thread (if any).                                          */
int32 post_sem(semaphore_t* sem) {
	lock_mutex(&MUTEX_LOCK);
	if(sem->state == S_FREE) {
		release_mutex(&MUTEX_LOCK);
		return -1;
	}
	++sem->queue.key;
	if(sem->queue.key <= 0) {
		int32 threadid = dequeue_thread(&sem->queue);
		release_mutex(&MUTEX_LOCK);
		resume_thread(threadid);
	} else {
		release_mutex(&MUTEX_LOCK);
	}
  	return 0;
}
