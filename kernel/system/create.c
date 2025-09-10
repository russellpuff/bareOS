#include <lib/barelib.h>
#include <system/thread.h>
#include <system/interrupts.h>
#include <system/semaphore.h>

static void trampoline(void) {
  asm volatile("sret");
}

thread_t thread_table[NTHREADS];  /*  Create a table of threads (one extra for the EMPTY proc */
uint32_t current_thread = 0;        /*  Set the initial thread id to 0                          */

/*  `wrapper` acts as a decorator function for the thread's entry function.  *
 *  It ensures  that setup is performed  before the function  is called and  *
 *  cleanup is performed after it completes.                                 */
void wrapper(byte (*proc)(char*)) {
  char* arg = (char*)thread_table[current_thread].stackptr;
  thread_table[current_thread].retval = proc(arg);  /*  Call the thread's entry point function and store the result on return       */
  kill_thread(current_thread);                      /*  Clean up thread after completion                                            */
}

/*  `create_thread`  takes a pointer  to a function that  acts as the entry  *
 *  point for a thread and selects an unused entry in the thread table.  It  *
 *  configures this  entry to represent a newly  created thread running the  *
 *  entry point function and places it in the suspended state.               */
int32_t create_thread(void* proc, char* arg, uint32_t arglen) {
  byte* stkptr;
  uint64_t i, j, pad, *ctxptr;
  for (i=0; i<NTHREADS && thread_table[i].state != TH_FREE; i++); /*  Find the first TH_FREE entry in the thread table    */
  if (i == NTHREADS)                                              /*                                                      */
    return -1;                                                    /*  Terminate is there are no free thread entries       */

  if (arglen > THREAD_STACK_SZ / 2)                               /*  Ensure the argument does not overrun the thread's  */
    return -2;                                                    /*  stack                                              */
  
  stkptr = ((byte*)get_stack(i));
  pad = (arglen % 4 ? arglen % 4 : 4);           /*  Align argument with between 1 and 4 \0 chars       */
  
  stkptr = stkptr - (pad + arglen);              /*  Push the top of the stock down below the args  */
  for (j=0; j<arglen; j++) stkptr[j] = arg[j];   /*  Copy arg above of the stack                    */
  for (; j<pad+arglen; j++) stkptr[j] = '\0';    /*  Pad top of stack with 0s to prevent overflow   */
  
  ctxptr = (uint64_t*)stkptr;
  thread_table[i].priority = 0;                /*                                                                */
  thread_table[i].state = TH_SUSPEND;          /*                                                                */
  thread_table[i].stackptr = (uint64_t*)stkptr;  /*              Configure the thread table entry                  */
  thread_table[i].parent = current_thread;     /*                                                                */
  thread_table[i].sem = create_sem(0);
  ctxptr[-1] = (uint64_t)trampoline;             /*  [-1] Return address after context switch in Machine privilage */
  ctxptr[-2] = (uint64_t)proc;                   /*  [-2] 'a0' register or first argument to the wrapper function  */
  ctxptr[-3] = (uint64_t)wrapper;                /*  [-3] Return point after returning from system call            */

  return i;
}
