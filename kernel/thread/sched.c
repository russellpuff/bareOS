#include <system/thread.h>
#include <system/syscall.h>

/*  'resched' places the current running thread into the ready state  *
 *  and  places it onto  the tail of the  ready queue.  Then it gets  *
 *  the head  of the ready  queue  and sets this  new thread  as the  *
 *  'current_thread'.  Finally,  'resched' uses 'ctxsw' to swap from  *
 *  the old thread to the new thread.                                 */
void resched(void) {
    uint32_t new_thread = dequeue_thread(&ready_list);
    if (new_thread == -1) return;
    if (thread_table[new_thread].root_ppn == NULL) {
        // TODO: panic
        return;
    }
    uint32_t old_thread = current_thread;
    current_thread = new_thread;
    thread_table[new_thread].state = TH_RUNNING;
    if (thread_table[old_thread].state == TH_RUNNING ||
        thread_table[old_thread].state == TH_READY) {
        thread_table[old_thread].state = TH_READY;
        queue_table[old_thread].key = thread_table[old_thread].priority;
        enqueue_thread(&ready_list, old_thread);
    }
    ctxsw(&thread_table[new_thread], &thread_table[old_thread]);
    return;
}

/*  Takes a index into the thread table of a thread to resume.  If the thread is already  *
 *  ready  or running,  returns an error.  Otherwise, adds the thread to the ready list,  *
 *  sets  the thread's  state to  ready and raises a RESCHED  syscall to  schedule a new  *
 *  thread.  Returns the threadid to confirm resumption.                                  */
int32_t resume_thread(uint32_t threadid) {
    if (threadid >= NTHREADS || (thread_table[threadid].state != TH_SUSPEND && thread_table[threadid].state != TH_WAITING))
        return -1;
    if (thread_table[threadid].root_ppn == NULL) {
        // TODO: panic
        return -1;
    }
    thread_table[threadid].state = TH_READY;
    enqueue_thread(&ready_list, threadid);
    raise_syscall(RESCHED);
    return threadid;
}

/*  Takes a index into the thread table of a thread to suspend.  If the thread is  *
 *  not in the  running or  ready state,  returns an error.   Otherwise, sets the  *
 *  thread's  state  to  suspended  and  raises a  RESCHED  syscall to schedule a  *
 *  different thread.  Returns the threadid to confirm suspension.                 */
int32_t suspend_thread(uint32_t threadid) {
    if (threadid < 0 || threadid >= NTHREADS ||
        (thread_table[threadid].state != TH_RUNNING &&
            thread_table[threadid].state != TH_READY)) {
        return -1;
    }

    if (thread_table[threadid].state == TH_READY) {
        detach_thread(threadid, false);
    }

    thread_table[threadid].state = TH_SUSPEND;
    raise_syscall(RESCHED);
    return threadid;
}

/*  Places the thread into a sleep state and inserts it into the  *
 *  sleep delta list.                                             */
int32_t sleep_thread(uint32_t threadid, uint32_t delay) {
    if (threadid >= NTHREADS || thread_table[threadid].state != TH_READY) return -1;
    detach_thread(threadid, false);
    thread_table[threadid].state = TH_SLEEP;
    queue_t* node = &queue_table[threadid];
    node->key = delay;
    delta_enqueue(&sleep_list, threadid);
    raise_syscall(RESCHED);
    return 0;
}

/*  If the thread is in the sleep state, remove the thread from the  *
 *  sleep queue and resumes it.                                      */
int32_t unsleep_thread(uint32_t threadid) {
    if (threadid >= NTHREADS || thread_table[threadid].state != TH_SLEEP) return -1;
    detach_thread(threadid, true);
    queue_t* node = &queue_table[threadid];
    node->key = thread_table[threadid].priority;
    thread_table[threadid].state = TH_READY;
    enqueue_thread(&ready_list, threadid);
    raise_syscall(RESCHED);
    return 0;
}