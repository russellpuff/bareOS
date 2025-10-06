#include <system/thread.h>
#include <system/interrupts.h>
#include <mm/vm.h>
#include <lib/string.h>

thread_t thread_table[NTHREADS];  /*  Create a table of threads  */
uint32_t current_thread = 0;        /*  Set the initial thread id to 0                          */
queue_t sleep_list;

/*
 *  'thread_init' sets up the thread table so that each thread is
 *  ready to be started.  It also sets  the init  thread (running
 *  the 'start'  function) to be  running as the  current  thread.
 */
void init_threads(void) {
    for (int i = 0; i < NTHREADS; i++) {
        thread_table[i].stackptr = NULL;
        thread_table[i].priority = 0;
        thread_table[i].parent = NTHREADS;
        thread_table[i].asid = i;
        thread_table[i].state = TH_FREE;
        thread_table[i].sem = create_sem(0);
    }
    current_thread = 0;
    thread_table[current_thread].state = TH_RUNNING;
    thread_table[current_thread].priority = (uint32_t)-1;
}

static void trampoline(void) {
    asm volatile("sret");
}

/*  `wrapper` acts as a decorator function for the thread's entry function.  *
 *  It ensures  that setup is performed  before the function  is called and  *
 *  cleanup is performed after it completes.                                 */
void wrapper(byte(*proc)(char*)) {
    char* arg = (char*)thread_table[current_thread].stackptr;
    thread_table[current_thread].retval = proc(arg);  /*  Call the thread's entry point function and store the result on return       */
    kill_thread(current_thread);                      /*  Clean up thread after completion                                            */
}

/*  `create_thread`  takes a pointer  to a function that  acts as the entry  *
 *  point for a thread and selects an unused entry in the thread table.  It  *
 *  configures this  entry to represent a newly  created thread running the  *
 *  entry point function and places it in the suspended state.               */
int32_t create_thread(void* proc, char* arg, uint32_t arglen) {
    uint64_t i, pad;
    byte* stkptr_pa;

    for (i = 0; i < NTHREADS && thread_table[i].state != TH_FREE; i++); /*  Find the first TH_FREE entry in the thread table    */
    if (i == NTHREADS)                                                  /*                                                      */
        return -1;                                                      /*  Terminate is there are no free thread entries       */

    /* Allocate per-thread address space for VM */
    uint64_t exec_pa, stack_pa;
    uint64_t root_ppn = alloc_page(ALLOC_PROC, &exec_pa, &stack_pa);
    if (root_ppn == NULL) return -1;

    const uint64_t STACK_VA_BASE = 0x200000UL;
    const uint64_t STACK_SIZE = 0x200000UL;
    if (arglen > (STACK_SIZE - 64)) return -2;

    pad = (arglen % 4 ? (4 - (arglen % 4)) : 0); /* Alignment for arg */
    stkptr_pa = (byte*)(stack_pa + STACK_SIZE - 16);
    stkptr_pa -= (pad + arglen); /* Reserve space for arg + padding */

    /* Copy arg into stack */
    memcpy(stkptr_pa, arg, arglen);
    memset(stkptr_pa + arglen, '\0', pad);

    uint64_t* ctxptr_pa = (uint64_t*)stkptr_pa;
    ctxptr_pa[-1] = (uint64_t)trampoline;       /*  [-1] Return address after context switch in Machine privilage */
    ctxptr_pa[-2] = (uint64_t)proc;             /*  [-2] 'a0' register or first argument to the wrapper function  */
    ctxptr_pa[-3] = (uint64_t)wrapper;          /*  [-3] Return point after returning from system call            */

    /* Configure thread table entry */
    thread_table[i].root_ppn = root_ppn;
    thread_table[i].asid = (uint16_t)i;
    thread_table[i].stackptr = (uint64_t*)(STACK_VA_BASE + STACK_SIZE - 16 - (pad + arglen));
    thread_table[i].state = TH_SUSPEND;
    thread_table[i].priority = 0;
    thread_table[i].parent = current_thread;
    thread_table[i].sem = create_sem(0);
    return i;
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

/*  Takes an index into the thread_table.  If that thread is not free (in use),  *
 *  sets the thread to defunct and raises a RESCHED syscall.                     */
int32_t kill_thread(uint32_t threadid) {
    if (threadid >= NTHREADS || thread_table[threadid].state == TH_FREE) /*                                                             */
        return -1;                                                         /*  Return if the requested thread is invalid or already free  */

    for (int i = 0; i < NTHREADS; i++) {               /*                                        */
        if (thread_table[i].parent == threadid)        /*  Identify all children of the thread   */
            thread_table[i].state = TH_FREE;           /*  Reap running children threads         */
    }
    thread_table[threadid].state = TH_DEFUNCT;         /*  Set the thread's state to TH_DEFUNCT  */
    if (thread_table[threadid].root_ppn != kernel_root_ppn) {
        free_pages(thread_table[threadid].root_ppn);   /*  Free pages associated with thread     */
    }
    post_sem(&thread_table[threadid].sem); /* Notify waiting threads. */
    free_sem(&thread_table[threadid].sem); /* Calls resched after dumping children. */
    return 0;
}