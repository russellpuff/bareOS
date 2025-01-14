#include <barelib.h>
#include <sleep.h>
#include <queue.h>
#include "testing.h"

static tests_t tests[] = {
  {
    .category = "General",
    .prompts = {
      "Program Compiles",
      "Sleep queue initialized"
    }
  },
  {
    .category = "Sleep",
    .prompts = {
      "Error on invalid thread insertion",
      "Thread state set on sleep",
      "Correct single sleep thread handling",
      "Correct suffix insertion into sleep list",
      "Correct prefix insertion into sleep list",
      "Correct infix insertion into sleep list",
    }
  },
  {
    .category = "Unsleep",
    .prompts = {
      "Error on invalid thread removal",
      "Thread state set to ready",
      "Correct suffix removal",
      "Correct prefix removal",
      "Correct infix removal"
    }
  },
  {
    .category = "Resched",
    .prompts = {
      "Sleep list decremented on clock tick",
      "Zero delay threads removed on clock tick",
      "Removed threads added to ready list"
    }
  },
};

#define INIT_ROOT_NULL 4
#define INIT_ROOT_PTR  8

static byte check_resched=false;
static int32 init_type;
static void order_check(int32 phase, int32 test, int32* delays, int32* order, int32 count, char* suffix) {
  queue_t* root = &sleep_list;
  if (check_resched)
    assert(t__analytics[RESCHED_REF].numcalls == 1, tests[phase].results[test], "s", "FAIL - resched not invoked after thread order change");
  t__reset_analytics();
  
  for (int i=0; i<count; i++) {
    int32 idx = (root->qnext - queue_table);
    assert(idx == order[i], tests[phase].results[test], "sds", "FAIL - qnext field does not match expcted order after ", count, suffix);
    root = root->qnext;
    assert(root->key == delays[i], tests[phase].results[test], "s", "FAIL - delay was not properly adjusted to maintain delta list");
  }
  if ((init_type & INIT_ROOT_NULL) && count == 0)
    assert(root->qnext == NULL, tests[phase].results[test], "s", "FAIL - queue was not empty");
  else
    assert(root->qnext == &sleep_list, tests[phase].results[test], "s", "FAIL - queue did not wrap back to 'sleep_list'");
  root = &sleep_list;

  for (int i=count-1; i>=0; i--) {
    int32 idx = (root->qprev - queue_table);
    assert(idx == order[i], tests[phase].results[test], "sds", "FAIL - qprev field does not match expected order after ", count, suffix);
    root = root->qprev;
  }
  if ((init_type & INIT_ROOT_NULL) && count == 0)
    assert(root->qprev == NULL, tests[phase].results[test], "s", "FAIL - queue was not empty");
  else
    assert(root->qprev == &sleep_list, tests[phase].results[test], "s", "FAIL - queue did not wrap back to 'sleep_list'");

  
}

static uint32 disabled(void) {
  return 1;
}

static uint32 pre_second_resched(void) {
  assert(sleep_list.qnext == &queue_table[3], tests[3].results[1],"s", "FAIL - Sleep queue did not remove all threads ready to be resumed");
  assert(sleep_list.qprev == &queue_table[3], tests[3].results[1], "s", "FAIL - Sleep queue did not remove all threads ready to be resumed");
  assert(thread_table[1].state == TH_READY, tests[3].results[2], "s", "FAIL - Awoken thread not set to TH_READY");
  assert(thread_table[2].state == TH_READY, tests[3].results[2], "s", "FAIL - Awoken thread not set to TH_READY");
  assert(thread_table[3].state == TH_SLEEP, tests[3].results[2], "s", "FAIL - Resumed extra thread");
  assert(ready_list.qnext == &queue_table[1], tests[3].results[2], "s", "FAIL - Thread was not added to ready list");
  assert(ready_list.qprev == &queue_table[2], tests[3].results[2], "s", "FAIL - Thread was not added to ready list");

  t__reset();
  return 1;
}

static uint32 pre_first_resched(void) {
  check_resched = false;
  assert(sleep_list.qnext == &queue_table[1], tests[3].results[0], "s", "FAIL - Timer removed thread prematurely");
  assert(queue_table[1].key == 10, tests[3].results[0], "s", "FAIL - Timer did not decrement timer by correct amount");
  assert(queue_table[2].key == 0, tests[3].results[0], "s", "FAIL - Timer decremented non-head nodes");
  assert(queue_table[3].key == 30, tests[3].results[0], "s", "FAIL - Timer decremented non-head nodes");
  t__analytics[RESCHED_REF].precall = (uint32 (*)())pre_second_resched;
  return 1;
}

static uint32 setup_general(void) {
  assert(1, tests[0].results[0], "");
  
  init_type |= (sleep_list.qnext == NULL ? INIT_ROOT_NULL : INIT_ROOT_PTR);
  assert(sleep_list.qnext == ((init_type & INIT_ROOT_NULL) ? NULL : &sleep_list), tests[0].results[1], "s", "FAIL - sleep_list 'qnext' is not expected value");
  assert(sleep_list.qprev == ((init_type & INIT_ROOT_NULL) ? NULL : &sleep_list), tests[0].results[1], "s", "FAIL - sleep_list 'qprev' is not expected value");
  t__reset();
  return 1;
}

static uint32 setup_sleep(void) {
  int32 result;
  // Setup threads [1: READY, 2: READY, 3: READY, 4: READY]
  for (int i=1; i<5; i++) {
    thread_table[i].state = TH_READY;
    enqueue_thread(&ready_list, i);
  }
  // Test out of bounds thread
  result = sleep_thread(NTHREADS, 100);
  assert(result == -1, tests[1].results[0], "s", "FAIL - Incorrect return value for out of bounds insertion");
  order_check(1, 0, NULL, NULL, 0, " insertion(s)");
  
  // Test invalid thread
  thread_table[7].state = TH_FREE;
  result = sleep_thread(7, 80);
  assert(result == -1, tests[1].results[0], "s", "FAIL - Incorrect return value when inserting a TH_FREE thread");
  order_check(1, 0, NULL, NULL, 0, " insertion(s)");
  
  // Test simple add [2: 98]  { 2: 98 }
  check_resched = true;
  result = sleep_thread(2, 98);
  assert(thread_table[2].state == TH_SLEEP, tests[1].results[1], "s", "FAIL - Thread was not set to TH_SLEEP");
  assert(result != -1, tests[1].results[2], "s", "FAIL - Returned error on a successful insertion operation");
  order_check(1, 2, (int32[]){ 98 }, (int32[]){ 2 }, 1, " insertion");
  
  // Test suffix add [4: 145] { 2: 98, 4: 47 }
  result = sleep_thread(4, 145);
  assert(thread_table[4].state == TH_SLEEP, tests[1].results[1], "s", "FAIL - Thread was not set to TH_SLEEP");
  assert(result != -1, tests[1].results[3], "s", "FAIL - Returned error on a successful insertion operation");
  order_check(1, 3, (int32[]){ 98, 47 }, (int32[]){ 2, 4 }, 2, " insertions");
  
  // Test prefix add [1: 80]  { 1: 80, 2: 18, 4:47 }
  result = sleep_thread(1, 80);
  assert(thread_table[1].state == TH_SLEEP, tests[1].results[1], "s", "FAIL - Thread was not set to TH_SLEEP");
  assert(result != -1, tests[1].results[4], "s", "FAIL - Returned error on a successful insertion operation");
  order_check(1, 4, (int32[]){ 80, 18, 47 }, (int32[]){ 1, 2, 4 }, 3, " insertions");
  
  // Test infix add  [3: 138] { 1: 80, 2: 18, 3: 40, 4: 7 }
  result = sleep_thread(3, 138);
  assert(thread_table[3].state == TH_SLEEP, tests[1].results[1], "s", "FAIL - Thread was not set to TH_SLEEP");
  assert(result != -1, tests[1].results[4], "s", "FAIL - Returned error on a successful insertion operation");
  order_check(1, 5, (int32[]){ 80, 18, 40, 7 }, (int32[]){ 1, 2, 3, 4 }, 4, " insertions");  

  check_resched = false;
  // Test out of bounds unsleep
  result = unsleep_thread(NTHREADS);
  assert(result == -1, tests[2].results[0], "s", "FAIL - Incorrect return value after out of bounds removal");
  order_check(2, 0, (int32[]){ 80, 18, 40, 7 }, (int32[]){ 1, 2, 3, 4 }, 4, " removals");

  thread_table[8].state = TH_READY;
  // Test invalid thread unsleep
  result = unsleep_thread(8);
  assert(result == -1, tests[2].results[0], "s", "FAIL - Incorrect return value when removing TH_READY thread");
  order_check(2, 0, (int32[]){ 80, 18, 40, 7 }, (int32[]){ 1, 2, 3, 4 }, 4, " removals");

  check_resched = true;
  // Test suffix unsleep [4] { 1: 80, 2: 18, 3: 40 }
  result = unsleep_thread(4);
  assert(thread_table[4].state == TH_READY, tests[2].results[1], "s", "FAIL - Thread was not set to TH_READY");
  assert(queue_table[4].qnext != NULL && queue_table[4].qnext != &queue_table[4], tests[2].results[2],
	 "s", "FAIL - Thread not added to ready_list");
  assert(queue_table[4].qprev != NULL && queue_table[4].qprev != &queue_table[4], tests[2].results[2],
	 "s", "FAIL - Thread not added to ready_list");
  order_check(2, 2, (int32[]){ 80, 18, 40 }, (int32[]){ 1, 2, 3 }, 3, " removals");

  // Test infix unsleep
  result = unsleep_thread(2);
  assert(thread_table[2].state == TH_READY, tests[2].results[1], "s", "FAIL - Thread was not set to TH_READY");
  assert(queue_table[2].qnext != NULL && queue_table[2].qnext != &queue_table[2], tests[2].results[4],
	 "s", "FAIL - Thread not added to ready_list");
  assert(queue_table[2].qprev != NULL && queue_table[2].qprev != &queue_table[2], tests[2].results[4],
	 "s", "FAIL - Thread not added to ready_list");
  order_check(2, 4, (int32[]){ 80, 58 }, (int32[]){ 1, 3 }, 2, " removals");

  // Test prefix unsleep
  result = unsleep_thread(1);
  assert(thread_table[1].state == TH_READY, tests[2].results[1], "s", "FAIL - Thread was not set to TH_READY");
  assert(queue_table[1].qnext != NULL && queue_table[1].qnext != &queue_table[1], tests[2].results[3],
	 "s", "FAIL - Thread not added to ready_list");
  assert(queue_table[1].qprev != NULL && queue_table[1].qprev != &queue_table[1], tests[2].results[3],
	 "s", "FAIL - Thread not added to ready_list");
  order_check(2, 3, (int32[]){ 138 }, (int32[]){ 3 }, 1, " removal");
  
  t__reset();
  return 1;
}

static uint32 setup_unsleep(void) {
  /*  Testing for this is handled during sleep for simplicity (still need this for test synchronization)  */  
  t__reset();
  return 1;
}

static uint32 setup_resched(void) {
  // Setup Slept threads
  thread_table[1].state = TH_SLEEP;
  thread_table[2].state = TH_SLEEP;
  thread_table[3].state = TH_SLEEP;

  queue_table[1].key = 20;
  queue_table[1].qprev = &sleep_list;
  queue_table[1].qnext = &queue_table[2];
  queue_table[2].key = 0;
  queue_table[2].qprev = &queue_table[1];
  queue_table[2].qnext = &queue_table[3];
  queue_table[3].key = 30;
  queue_table[3].qprev = &queue_table[2];
  queue_table[3].qnext = &sleep_list;
  
  sleep_list.qnext = &queue_table[1];
  sleep_list.qprev = &queue_table[3];
  
  t__analytics[RESCHED_REF].precall = (uint32 (*)())pre_first_resched;
  t__analytics[DELEG_CLK_REF].precall = (uint32 (*)())NULL; 
  while (1) {

  }
  
  t__reset();
  return 1;
}

void t__ms5(uint32 run) {
  switch (run) {
  case 0:
    t__initialize_tests(tests, 4);
    t__analytics[RESCHED_REF].precall = (uint32 (*)())disabled;
    t__analytics[DELEG_CLK_REF].precall = (uint32 (*)())disabled;
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())setup_general; break;
  case 1:
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())setup_sleep; break;
  case 2:
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())setup_unsleep; break;
  case 3:
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())setup_resched; break;
    
  }
}
