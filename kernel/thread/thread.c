#include <system/thread.h>
#include <system/interrupts.h>
#include <system/syscall.h>
#include <system/panic.h>
#include <mm/vm.h>
#include <lib/string.h>
#include <system/queue.h>
#include <mm/malloc.h>

#define SSTATUS_SIE   (1UL << 1)
#define SSTATUS_SPIE  (1UL << 5)
#define SSTATUS_SPP   (1UL << 8)   /* 1 = S-mode */
#define SSTATUS_SUM   (1UL << 18)
#define SSTATUS_MXR   (1UL << 19)

thread_t thread_table[NTHREADS];  /*  Create a table of threads  */
uint32_t current_thread; 
queue_t sleep_list;
uint16_t next_asid;

/*
 *  'thread_init' sets up the thread table so that each thread is
 *  ready to be started.  It also sets  the init  thread (running
 *  the 'start'  function) to be  running as the  current  thread.
 */
void init_threads(void) {
    for (uint32_t i = 0; i < NTHREADS; i++) {
        thread_table[i].stackptr = NULL;
        thread_table[i].priority = 0;
        thread_table[i].parent = NTHREADS;
        thread_table[i].asid = i;
        thread_table[i].state = TH_FREE;
        thread_table[i].sem = create_sem(0);
        thread_table[i].mode = MODE_S;
    }
    next_asid = 1;
}

/* This is where processes wait to be reaped. It's its own function for debugging clarity. */
void wait_for_reaper(void) {
    while (1);
}

void landing_pad(void) {
    trapret(thread_table[current_thread].tf);
}

/*  `wrapper` acts as a decorator function for the thread's entry function.  *
 *  It ensures  that setup is performed  before the function  is called and  *
 *  cleanup is performed after it completes.                                 */
void wrapper(byte(*proc)(char*)) {
    char* arg = thread_table[current_thread].argptr;
    thread_table[current_thread].retval = proc(arg);  /*  Call the thread's entry point function and store the result on return */
    thread_table[current_thread].state = TH_ZOMBIE;
    if(thread_table[current_thread].argptr != NULL) free(thread_table[current_thread].argptr);
    thread_table[current_thread].argptr = NULL;
    enqueue_thread(&reap_list, current_thread);
    wait_for_reaper();
}

/* user_thread_exit is a jump point after a user process sends an ecall to exit */
void user_thread_exit(trapframe* tf) {
    thread_table[current_thread].retval = (byte)(tf->a0 & 0xFF);
    thread_table[current_thread].state = TH_ZOMBIE;
    if (thread_table[current_thread].argptr != NULL) {
        free(thread_table[current_thread].argptr);
        thread_table[current_thread].argptr = NULL;
    }
    enqueue_thread(&reap_list, current_thread);
    tf->sstatus |= SSTATUS_SPP | SSTATUS_SPIE;
    tf->sepc = (uint64_t)wait_for_reaper;
}

/*  `create_thread`  takes a pointer  to a function that  acts as the entry  *
 *  point for a thread and selects an unused entry in the thread table.  It  *
 *  configures this  entry to represent a newly  created thread running the  *
 *  entry point function and places it in the suspended state.               */
int32_t create_thread(void* proc, char* arg, uint32_t arglen, thread_mode mode) {
    uint64_t new_id;
    /* Statically allocated by the simple allocator */
    const uint64_t STACK_BASE = 0x200000UL;
    const uint64_t STACK_SIZE = 0x200000UL;

    for (new_id = 0; new_id < NTHREADS && thread_table[new_id].state != TH_FREE; new_id++); /*  Find the first TH_FREE entry in the thread table    */
    if (new_id == NTHREADS) {
        panic("No free thread entries in the thread table for this new thread. No handler exists to wait for one to become free.\n");
    }

    /* Allocate per-thread address space for VM */
    uint64_t root_ppn = alloc_page(new_id);
    if (root_ppn == NULL) {
        panic("Page allocator failed to allocate a page for this new thread.\n");
    }
    
    if (arglen > (STACK_SIZE - 64)) {
        panic("Arglen was too big for the statically allocated stack size for this new thread.\n");
    }
    
    if (arglen > 0) {
        thread_table[new_id].argptr = malloc(arglen);
        if (thread_table[new_id].argptr == NULL) {
            panic("Couldn't malloc enough space for the new thread's arg.\n");
        }
        memcpy(thread_table[new_id].argptr, arg, arglen);
    }
    else {
        thread_table[new_id].argptr = NULL;
    }

    /* Layout: [ kernel stack … ][ trapframe ][ context ][TOP] */
    byte* ktop = thread_table[new_id].kstack_top; /* Virtual if MMU on, physical if MMU off */
    if (ktop == NULL) {
        panic("kstack top pointer is null\n");
    }
    
    context* ctx = (context*)(ktop - sizeof(context));
    trapframe* tf = (trapframe*)((byte*)ctx - sizeof(trapframe));

    memset(ctx, 0, sizeof(context));
    ctx->sp = (uint64_t)tf;  /* where kernel SP should be after a switch */
    ctx->ra = (uint64_t)landing_pad; /* Starter ret landing pad for new threads */

    memset(tf, 0, sizeof(trapframe));
    if (mode == MODE_S) {
        tf->sstatus = SSTATUS_SPP | SSTATUS_SPIE | SSTATUS_SUM;
        tf->sepc = (uint64_t)wrapper;             /* first PC */
        tf->a0 = (uint64_t)proc;                  /* wrapper(proc) */
        tf->ra = (uint64_t)wait_for_reaper;       /* if wrapper ever returns */
    }
    else {
        tf->sstatus = SSTATUS_SPIE | SSTATUS_SUM;
        tf->sepc = (uint64_t)proc;
        tf->ra = 0;
    }
    tf->sp = STACK_BASE + STACK_SIZE - 0x20;
    
    /* Publish into thread record */
    thread_table[new_id].tf = tf;
    thread_table[new_id].ctx = ctx;
    thread_table[new_id].stackptr = (uint64_t*)ktop;
    thread_table[new_id].root_ppn = root_ppn;
    thread_table[new_id].asid = next_asid++;
    thread_table[new_id].state = TH_SUSPEND;
    thread_table[new_id].priority = 0;
    thread_table[new_id].parent = current_thread;
    thread_table[new_id].sem = create_sem(0);
    thread_table[new_id].mode = mode;

    /* First thread means these were set to physical and have to be virtual after being loaded */
    if (!MMU_ENABLED) {
        thread_table[new_id].kstack_base += KVM_BASE;
        thread_table[new_id].kstack_top += KVM_BASE;
    }
    
    return new_id;
}

/*  Takes an index into the thread_table.  If the thread is TH_DEFUNCT,  *
 *  mark  the thread  as TH_FREE  and return its  `retval`.   Otherwise  *
 *  raise RESCHED and loop to check again later.                         */
int32_t join_thread(uint32_t threadid) {
    if (threadid >= NTHREADS || thread_table[threadid].state == TH_FREE) {
        return -1;
    }

    if (thread_table[threadid].state != TH_DEFUNCT) {
        wait_sem(&thread_table[threadid].sem);
    }

    thread_table[threadid].state = TH_FREE;
    return thread_table[threadid].retval;
}

/* Takes an index into the thread table and marks the thread as defunct and *
 * frees up any resources it was using. This can only be called by the      *
 * scheduler looking through the reap_list of to-be-killed threads after    *
 * their context is swapped from for the last time. So there's no possible  *
 * chance of context corruption from freeing pages                          */
int32_t kill_thread(uint32_t thread_id) {
    if (thread_id >= NTHREADS || thread_table[thread_id].state == TH_FREE) /*                                                             */
        return -1;                                                       /*  Return if the requested thread is invalid or already free  */

    /* TODO: This is lousy and unsafe. Doesn't properly reap anything. */
    /* Actually don't do this at all, this kills the idle and shell when root is reaped LOL 
    
    for (uint32_t i = 0; i < NTHREADS; ++i) {
        if (thread_table[i].parent == threadid) {
            thread_table[i].state = TH_FREE;
        }
    }
    */
    /* Free in case of premature kill */
    if (thread_table[thread_id].argptr != NULL) {
        free(thread_table[thread_id].argptr);
        thread_table[thread_id].argptr = NULL;
    }

    if (thread_table[thread_id].root_ppn != kernel_root_ppn) {
        free_process_pages(thread_id);   /*  Free pages associated with thread     */
    }
    thread_table[thread_id].root_ppn = NULL;
    post_sem(&thread_table[thread_id].sem); /* Notify waiting threads. */
    free_sem(&thread_table[thread_id].sem); /* Calls resched after dumping children. */

    thread_table[thread_id].state = TH_DEFUNCT;         /*  Set the thread's state to TH_DEFUNCT  */
    return 0;
}