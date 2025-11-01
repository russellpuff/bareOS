#include <system/thread.h>
#include <system/syscall.h>
#include <system/panic.h>
#include <mm/vm.h>

// todo: these panic wrongly if passed an invalid thread id, fix eventually
static uint64_t get_satp(uint16_t asid, uint64_t root_ppn) {
    return (8UL << 60) | ((uint64_t)asid << 44) | (root_ppn & ((1UL << 44) - 1));
}

void context_switch(thread_t* next, thread_t* prev) {
    uint64_t satp = get_satp(next->asid, next->root_ppn);
    ctxsw(prev->ctx, next->ctx, satp, (uint64_t)next->kstack_top);
}

void context_load(thread_t* first, uint32_t tid) {
    current_thread = tid;
    thread_table[current_thread].state = TH_RUNNING;
    uint64_t satp = get_satp(first->asid, first->root_ppn);
    if ((uint64_t)first->kstack_top < KVM_BASE) {
        panic("first ktop wasn't virtual\n");
    }
    first->ctx->sp = (uint64_t)first->kstack_top - sizeof(trapframe);
    ctxload(satp, (uint64_t)first->kstack_top);
    first->tf = (trapframe*)((uint64_t)first->tf + KVM_BASE);
    first->ctx = (context*)((uint64_t)first->ctx + KVM_BASE);
    trapret(first->tf);
}

/*  'resched' places the current running thread into the ready state  *
 *  and  places it onto  the tail of the  ready queue.  Then it gets  *
 *  the head  of the ready  queue  and sets this  new thread  as the  *
 *  'current_thread'.  Finally,  'resched' uses 'context_switch' to swap from  *
 *  the old thread to the new thread.                                 */

#include <system/semaphore.h>
extern void handle_syscall(uint64_t*);

void resched(void* caller) {
    /* DO NOT CHANGE THIS LIST OF APPROVED CALLERS!                *
     * THERE IS ZERO REASON TO EVER CHANGE THIS LIST!              *
     * IF YOU WANT TO CHANGE THIS LIST, TOO BAD!                   *
     * CALLING resched() OUTSIDE OF AN APPROVED CONTEXT IS UNSAFE! *
     * DANGEROUSLY UNSAFE, YOU WILL BREAK THE KERNEL!              *
     * UNLESS YOU GET APPROVAL FROM THE REPO OWNER (Robin),        *
     * DO NOT CHANGE THE FUCKING LIST!                             */
    if (caller != &handle_syscall && caller != &wait_sem) /* This is a weak check, if you spoof this to violate policy I'm going to break your kneecaps */
        panic("Policy violation detected, the scheduler was prompted in an inappropriate context.");

    uint32_t new_thread = dequeue_thread(&ready_list);
    if (new_thread == -1) return;

    if (!MMU_ENABLED) {
        panic("Can't resched - MMU is not enabled.");
    }

    if (thread_table[new_thread].root_ppn == NULL) {
        panic("A thread in the ready list had a null root ppn and was going to be resumed.\n");
    }

    uint32_t old_thread = current_thread;
    current_thread = new_thread;
    thread_table[new_thread].state = TH_RUNNING;

    if (thread_table[old_thread].state == TH_RUNNING || thread_table[old_thread].state == TH_READY) {
        thread_table[old_thread].state = TH_READY;
        queue_table[old_thread].key = thread_table[old_thread].priority;
        enqueue_thread(&ready_list, old_thread);
    }

    context_switch(&thread_table[new_thread], &thread_table[old_thread]);
}

void reaper(void) {
    while (1) {
        while (reap_list.qnext != &reap_list) {
            int32_t id = dequeue_thread(&reap_list);
            if (id == -1) continue; /* Encountered a phantom thread. Not worth panicking over for now. */
            kill_thread(id);
        }
    }
}

/*  Takes a index into the thread table of a thread to resume.  If the thread is already  *
 *  ready  or running,  returns an error.  Otherwise, adds the thread to the ready list,  *
 *  sets  the thread's  state to  ready and raises a RESCHED  syscall to  schedule a new  *
 *  thread.  Returns the threadid to confirm resumption.                                  */
int32_t resume_thread(uint32_t threadid) {
    if (threadid >= NTHREADS || (thread_table[threadid].state != TH_SUSPEND && thread_table[threadid].state != TH_WAITING)) {
        panic("Tried to resume a thread that was already ready/running.\n");
    }
    if (thread_table[threadid].root_ppn == NULL) {
        panic("A thread in the ready list had a null root ppn and was going to be resumed.\n");
    }
    thread_table[threadid].state = TH_READY;
    enqueue_thread(&ready_list, threadid);
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
        panic("Tried to suspend a thread that was already suspended.\n");
    }

    if (thread_table[threadid].state == TH_READY) {
        detach_thread(threadid, false);
    }

    thread_table[threadid].state = TH_SUSPEND;
    return threadid;
}

/*  Places the thread into a sleep state and inserts it into the  *
 *  sleep delta list.                                             */
int32_t sleep_thread(uint32_t threadid, uint32_t delay) {
    if (threadid >= NTHREADS || thread_table[threadid].state != TH_READY) {
        panic("Tried to sleep a thread that wasn't ready.\n");
    }
    detach_thread(threadid, false);
    thread_table[threadid].state = TH_SLEEP;
    queue_t* node = &queue_table[threadid];
    node->key = delay;
    delta_enqueue(&sleep_list, threadid);
    return 0;
}

/*  If the thread is in the sleep state, remove the thread from the  *
 *  sleep queue and resumes it.                                      */
int32_t unsleep_thread(uint32_t threadid) {
    if (threadid >= NTHREADS || thread_table[threadid].state != TH_SLEEP) {
        panic("Tried to wake a thread that wasn't sleeping.\n");
    }
    detach_thread(threadid, true);
    queue_t* node = &queue_table[threadid];
    node->key = thread_table[threadid].priority;
    thread_table[threadid].state = TH_READY;
    enqueue_thread(&ready_list, threadid);
    return 0;
}