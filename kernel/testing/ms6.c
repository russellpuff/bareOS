#include <barelib.h>
#include <bareio.h>
#include <semaphore.h>
#include "testing.h"

static tests_t tests[] = {
  {
    .category = "General",
    .prompts = {
      "Program Compiles"
    },
  },
  {
    .category = "Create",
    .prompts = {
      "Returned semaphore is S_USED",
      "Returned semaphore's queue is empty",
      "Semaphore size reflects argument"
    },
  },
  {
    .category = "Free",
    .prompts = {
      "Awaiting threads are resumed",
      "Semaphore's state set to S_FREE"
    },
  },
  {
    .category = "Post",
    .prompts = {
      "Post is guarded with mutex",
      "Post increments semaphore",  // Test sem=-1, sem=0 and sem=1
      "Post reconfigures thread's state",
      "Post resumes one awaiting thread",
      "Post invokes resched"
    },
  },
  {
    .category = "Wait",
    .prompts = {
      "Wait is guarded with mutex",
      "Wait decrements semaphore", // Test sem=1, sem=0, sem=-1
      "Wait reconfigures thread's state",
      "Adds blocked threads to queue",
      "Multiple threads queued by semaphore",
      "Wait invokes resched"
    },
  },
};

static byte* mutex_ptr = NULL;
static byte test, phase;
static int32 test_mutex(byte* ptr) {
  if (mutex_ptr == NULL)
    mutex_ptr = ptr;
  return (mutex_ptr == ptr);
}

static uint32 pre_lock(byte* ptr) {
  assert(test_mutex(ptr), tests[phase].results[test], "s", "FAIL - Different mutex used for semaphore operation");
  return 0;
}

static uint32 pre_release(byte* ptr) {
  assert(test_mutex(ptr), tests[phase].results[test], "s", "FAIL - Different mutex used for semaphore operation");
  return 0;
}

static uint32 disabled(void) {
  return 1;
}

static uint32 mutex_resched(void) {
  assert(t__analytics[MUTEX_LOCK_REF].numcalls == 1, tests[phase].results[0], "s", "FAIL - Mutex was not locked during operation");
  assert(t__analytics[MUTEX_RELEASE_REF].numcalls == 1, tests[phase].results[0], "s", "FAIL - Mutex was not freed before resched");
  return 1;
}

static uint32 setup_general(void) {
  assert(1, tests[0].results[0], "");
  t__reset();
  return 1;
}

static uint32 setup_create(void) {
  semaphore_t sem = create_sem(5);

  assert(sem.state == S_USED, tests[1].results[0], "s", "FAIL - Semaphore not set to S_USED state");
  assert(sem.queue.key == 5, tests[1].results[2], "s", "FAIL - Semaphore count was not stored in queue");
  assert(sem.queue.qnext == NULL || sem.queue.qnext == &sem.queue, tests[1].results[1],
	 "s", "FAIL - Semaphore's 'qnext' field is not set to initial value");
  assert(sem.queue.qprev == NULL || sem.queue.qprev == &sem.queue, tests[1].results[1],
	 "s", "FAIL - Semaphore's 'qprev' field is not set to initial value");

  t__reset();
  return 1;
}

static uint32 setup_free(void) {
  semaphore_t sem = create_sem(0);

  sem.queue.key = -2;
  sem.queue.qnext = &queue_table[1];
  sem.queue.qprev = &queue_table[2];
  thread_table[1].state = TH_WAITING;
  thread_table[2].state = TH_WAITING;

  queue_table[1].qnext = &queue_table[2];
  queue_table[1].qprev = &sem.queue;
  queue_table[2].qnext = &sem.queue;
  queue_table[2].qprev = &queue_table[2];

  t__analytics[RESCHED_REF].precall = (uint32 (*)())mutex_resched;
  t__reset_analytics();
  free_sem(&sem);

  assert(sem.state == S_FREE, tests[2].results[1], "s", "FAIL - Semaphore was not set to S_FREE state");
  assert(thread_table[1].state == TH_READY, tests[2].results[0], "s", "FAIL - Thread was not correctly resumed");
  assert(thread_table[2].state == TH_READY, tests[2].results[0], "s", "FAIL - Thread was not correctly resumed");
  assert(ready_list.qnext == &queue_table[1], tests[2].results[0], "s", "FAIL - Thread was not correctly resumed");
  assert(ready_list.qprev == &queue_table[2], tests[2].results[0], "s", "FAIL - Thread was not correctly resumed");
  assert(t__analytics[RESCHED_REF].numcalls > 0, tests[2].results[0], "s", "FAIL - Resched was not invoked");

  t__reset_analytics();
  int32 result = free_sem(&sem);
  assert(result == -1, tests[1].results[1], "s", "FAIL - Does not return -1 when freeing S_FREE semaphore");

  t__reset();
  return 1;
}

static uint32 setup_post(void) {
  semaphore_t sem = create_sem(0);
  
  post_sem(&sem);
  assert(sem.queue.key == 1, tests[3].results[1], "s", "FAIL - Semaphore did not increment past 0");

  sem.queue.key = -1;
  sem.queue.qnext = &queue_table[1];
  sem.queue.qprev = &queue_table[1];
  thread_table[1].state = TH_WAITING;
  queue_table[1].qnext = &sem.queue;
  queue_table[1].qprev = &sem.queue;
  t__reset_analytics();
  post_sem(&sem);
  assert(sem.queue.key == 0, tests[3].results[1], "s", "FAIL - Semaphore did not increment when resuming a thread");
  assert(thread_table[1].state == TH_READY, tests[3].results[2], "s", "FAIL - Semaphore did not update resumed thread's state");
  assert(queue_table[1].qprev == &ready_list, tests[3].results[3], "s", "FAIL - Thread was not correctly added to ready list");
  assert(queue_table[1].qnext == &ready_list, tests[3].results[3], "s", "FAIL - Thread was not correctly added to ready list");
  assert(ready_list.qnext == &queue_table[1], tests[3].results[3], "s", "FAIL - Thread was not correctly added to ready list");
  assert(ready_list.qprev == &queue_table[1], tests[3].results[3], "s", "FAIL - Thread was not correctly added to ready list");
  assert(t__analytics[RESCHED_REF].numcalls > 0, tests[3].results[4], "s", "FAIL - Resched was not invoked");

  t__reset();
  return 1;
}

static uint32 setup_wait(void) {
  semaphore_t sem = create_sem(2);

  t__reset_analytics();
  wait_sem(&sem);
  assert(sem.queue.key == 1, tests[4].results[1], "s", "FAIL - Semaphore was not decremented when greater than 0");

  t__reset_analytics();
  wait_sem(&sem);
  assert(sem.queue.key == 0, tests[4].results[1], "s", "FAIL - Semaphore was not decremented when greater than 0");

  current_thread = 1;
  thread_table[1].state = TH_RUNNING;
  
  t__reset_analytics();
  wait_sem(&sem);
  assert(sem.queue.key == -1, tests[4].results[1], "s", "FAIL - Semaphore was not decremented when thread was blocked");
  assert(thread_table[1].state == TH_WAITING, tests[4].results[2], "s", "FAIL - State not set to TH_WAITING");
  assert(sem.queue.qnext == &queue_table[1], tests[4].results[3], "s", "FAIL - Thread not added to semaphore's queue");
  assert(sem.queue.qprev == &queue_table[1], tests[4].results[3], "s", "FAIL - Thread not added to semaphore's queue");
  assert(t__analytics[RESCHED_REF].numcalls > 0, tests[4].results[5], "s", "FAIL - Resched was not invoked");

  current_thread = 2;
  thread_table[2].state = TH_RUNNING;
  t__reset_analytics();
  wait_sem(&sem);
  assert(sem.queue.key == -2, tests[4].results[1], "s", "FAIL - Semaphore was not decremented when multiple threads are blocked");
  assert(thread_table[2].state == TH_WAITING, tests[4].results[2], "s", "FAIL - State was not set to TH_WAITING");
  assert(sem.queue.qnext == &queue_table[1], tests[4].results[4], "s", "FAIL - Second thread was not appended to semaphore's queue");
  assert(sem.queue.qprev == &queue_table[2], tests[4].results[4], "s", "FAIL - Second thread was not appended to semaphore's queue");
  
  t__reset();
  return 1;
}

void t__ms6(uint32 run) {
  switch (run) {
  case 0:
    phase = 0;
    test = 0;
    t__initialize_tests(tests, 5);
    t__analytics[RESCHED_REF].precall = (uint32 (*)())disabled;
    t__analytics[DELEG_CLK_REF].precall = (uint32 (*)())disabled;
    t__analytics[MUTEX_LOCK_REF].precall = (uint32 (*)())pre_lock;
    t__analytics[MUTEX_RELEASE_REF].precall = (uint32 (*)())pre_release;
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())setup_general;
    break;
  case 1:
    phase = 1;
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())setup_create; break;
  case 2:
    phase = 2;
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())setup_free; break;
  case 3:
    phase = 3;
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())setup_post; break;
  case 4:
    phase = 4;
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())setup_wait; break;
  }
}
