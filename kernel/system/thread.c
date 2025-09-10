#include <lib/barelib.h>
#include <system/thread.h>

/*
 *  'thread_init' sets up the thread table so that each thread is
 *  ready to be started.  It also sets  the init  thread (running
 *  the 'start'  function) to be  running as the  current  thread.
 */
void init_threads(void) {
  for (int i=0; i<NTHREADS; i++) {
    thread_table[i].state = TH_FREE;
    thread_table[i].parent = NTHREADS;
    thread_table[i].priority = 0;
    thread_table[i].stackptr = (uint64_t*)get_stack(i);
	thread_table[i].sem = create_sem(0);
  }
  current_thread = 0;
  thread_table[current_thread].state = TH_RUNNING;
  thread_table[current_thread].priority = (uint32_t)-1;
}
