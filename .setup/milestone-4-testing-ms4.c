#include <barelib.h>
#include <thread.h>
#include <queue.h>
#include "testing.h"

static tests_t tests[] = {
  {
    .category = "General",
    .prompts = {
      "Program Compiles",
      "List is initialized"
    }
  },
  {
    .category = "Enqueue",
    .prompts = {
      "Maintains order after insertion",
      "Correctly handles multiple queued threads",
      "Handles bad thread ids",
      "Correctly orders by key value",
      "Handles out of order insertions"
    }
  },
  {
    .category = "Dequeue",
    .prompts = {
      "Dequeue first thread from list",
      "Dequeue second thread from list",
      "Dequeue last thread from list",
      "Dequeue from empty queue returns error"
    }
  },
  {
    .category = "Resched",
    .prompts = {
      "Enqueue thread on resume",
      "Dequeue thread on suspend",
      "Resched adjusts ready list"
    }
  }
};

#define INIT_LIST_NULL 1
#define INIT_LIST_PTR  2
#define INIT_ROOT_NULL 4
#define INIT_ROOT_PTR  8

static uint32 no_clock(void) {
  return 1;
}

static uint32 pre_ctxsw(uint64** new, uint64** old) {
  assert(new == &thread_table[5].stackptr, tests[3].results[2], "s", "FAIL - Resched did not switch to the expected first value in ready_list");
  assert(old == &thread_table[1].stackptr, tests[3].results[2], "s", "FAIL - Resched did not switch from the currently running thread");
  return 1;
}

static uint32 validate_resched(int32* result) {
  /* TODO - write validate for settings */
  
  return 1;
}

static byte phase;
static int32 init_type;
static void order_check(byte test, uint32* order, uint32 count, char* suffix) {
  queue_t* root = &ready_list;
  for (int i=0; i<count; i++) {
    int32 idx = (root->qnext - queue_table);
    assert(idx == order[i], tests[phase].results[test], "sds", "FAIL - qnext field does not match expcted order after ", count, suffix);
    root = root->qnext;
  }
  if ((init_type & INIT_ROOT_NULL) && count == 0)
    assert(root->qnext == NULL, tests[phase].results[test], "s", "FAIL - queue was not empty");
  else
    assert(root->qnext == &ready_list, tests[phase].results[test], "s", "FAIL - queue did not wrap back to 'ready_list'");
  root = &ready_list;

  for (int i=count-1; i>=0; i--) {
    int32 idx = (root->qprev - queue_table);
    assert(idx == order[i], tests[phase].results[test], "sds", "FAIL - qprev field does not match expected order after ", count, suffix);
    root = root->qprev;
  }
  if ((init_type & INIT_ROOT_NULL) && count == 0)
    assert(root->qprev == NULL, tests[phase].results[test], "s", "FAIL - queue was not empty");
  else
    assert(root->qprev == &ready_list, tests[phase].results[test], "s", "FAIL - queue did not wrap back to 'ready_list'");
}

static void insert_thread(byte test, uint32 tid, uint32 prio, uint32* order, uint32 count) {
  int32 result;
  thread_table[tid].priority = prio;
  thread_table[tid].state = TH_READY;

  result = enqueue_thread(&ready_list, tid);
  assert(result != -1, tests[1].results[test], "s", "FAIL - Function returned -1 on what should be successful call");
  order_check(test, order, count, " insertion(s)");
}

static void remove_thread(byte test, int32 expected, uint32* order, uint32 count) {
  int32 result = dequeue_thread(&ready_list);
  assert(result == expected, tests[2].results[test], "s", "FAIL - return value did not match expected result");
  order_check(test, order, count, " removal(s)");
}

static void setup_general(void) {
  assert(1, tests[0].results[0], "");

  init_type |= (queue_table[0].qnext == NULL ? INIT_LIST_NULL : INIT_LIST_PTR);
  init_type |= (ready_list.qnext == NULL ? INIT_ROOT_NULL : INIT_ROOT_PTR);

  for (int i=0; i<NTHREADS; i++) {
    if (init_type & INIT_LIST_NULL) {
      assert(queue_table[i].qnext == NULL, tests[0].results[1], "s", "FAIL - queue_table entry contains 'qnext' that does not match");
      assert(queue_table[i].qprev == NULL, tests[0].results[1], "s", "FAIL - queue_table entry contains 'qprev' that does not match");
    }
    else {
      assert(queue_table[i].qnext == &queue_table[i], tests[0].results[1], "s", "FAIL - queue_table entry contains 'qnext' that does not match");
      assert(queue_table[i].qnext == &queue_table[i], tests[0].results[1], "s", "FAIL - queue_table entry contains 'qprev' that does not match");
    }
  }
  assert(ready_list.qnext == ((init_type & INIT_ROOT_NULL) ? NULL : &ready_list), tests[0].results[1], "s", "FAIL - ready_list 'qnext' is not expected value");
  assert(ready_list.qprev == ((init_type & INIT_ROOT_NULL) ? NULL : &ready_list), tests[0].results[1], "s", "FAIL - ready_list 'qprev' is not expected value");
  t__reset();
}

static void setup_enqueue(void) {
  int32 result;
  uint32 order[5];
  phase = 1;
  // Test out of bounds threadid
  result = enqueue_thread(&ready_list, NTHREADS);
  assert(result == -1, tests[1].results[2], "s", "FAIL - Out of bound thread id did not return correct error code");

  // Add priority 0 (idx 1)
  order[0] = 1;
  insert_thread(0, 1, 0, order, 1);
  
  // Test re-add (idx 1)
  result = enqueue_thread(&ready_list, 1);
  assert(result == -1, tests[1].results[2], "s", "FAIL - Inserting a thread already in list did not return failure");

  // Add priority 0 (idx 2)
  order[1] = 2;
  insert_thread(1, 2, 0, order, 2);
  
  // Add priority 10 (idx 12)
  order[2] = 12;
  insert_thread(3, 12, 10, order, 3);

  // Add priority 4 (idx 5)
  order[2] = 5;
  order[3] = 12;
  insert_thread(4, 5, 4, order, 4);

  // Add priority 10 (idx 8)
  order[4] = 8;
  insert_thread(4, 8, 10, order, 5);
  
  phase = 2;
  // Remove first
  remove_thread(0, 1, &order[1], 4);
  queue_t* ptr = ((init_type & INIT_LIST_NULL) ? NULL : &queue_table[1]);
  assert(queue_table[1].qnext == ptr, tests[2].results[0], "s", "FAIL - Removed queue entry was not reset correctly");
  assert(queue_table[1].qprev == ptr, tests[2].results[0], "s", "FAIL - Removed queue entry was not reset correctly");
  
  // Remove second
  remove_thread(1, 2, &order[2], 3);
  
  // Remove third (manual)
  dequeue_thread(&ready_list);
  
  // Remove fourth (manual)
  dequeue_thread(&ready_list);
  
  // Remove fifth (manual)
  remove_thread(2, 8, &order[4], 0);
  
  // Remove -1
  result = dequeue_thread(&ready_list);
  assert(result == -1, tests[2].results[3], "s", "FAIL - Out of bound thread id did not return correct error code");
  t__reset();
}

static void setup_dequeue(void) {
  /*  Testing for this is handled during enqueue for simplicity (still need this for test synchronization)  */
  t__reset();
}

static void setup_resched(void) {
  phase = 3;
  uint32 order[4] = {5, 2, 3, 4};
  thread_table[5].state = TH_READY;
  thread_table[5].priority = 0;
  thread_table[2].state = TH_READY;
  thread_table[2].priority = 0;
  thread_table[3].state = TH_READY;
  thread_table[3].priority = 0;
  thread_table[4].state = TH_SUSPEND;
  thread_table[4].priority = 0;
  
  enqueue_thread(&ready_list, 5);
  enqueue_thread(&ready_list, 2);
  enqueue_thread(&ready_list, 3);

  resume_thread(4);
  order_check(1, order, 4, " resume");

  uint32 new_order[3] = {5, 2, 4};
  suspend_thread(3);
  order_check(0, new_order, 3, " suspend");
  queue_t* ptr = ((init_type & INIT_LIST_NULL) ? NULL : &queue_table[3]);
  assert(queue_table[3].qnext == ptr, tests[3].results[1], "s", "FAIL - Removed queue entry was not reset correctly");
  assert(queue_table[3].qprev == ptr, tests[3].results[1], "s", "FAIL - Removed queue entry was not reset correctly");
  
  t__analytics[CTXSW_REF].precall = (uint32 (*)())pre_ctxsw;
  t__analytics[RESCHED_REF].precall = NULL;

  thread_table[1].state = TH_RUNNING;
  thread_table[1].priority = 0;
  current_thread = 1;
  resched();

  uint32 resched_order[3] = {2, 4, 1};
  order_check(2, resched_order, 3, " resched");
  assert(current_thread == 5, tests[3].results[2], "s", "FAIL - Current thread did not match the new running thread");
  t__reset();
}

void t__ms4(uint32 run) {
  switch (run) {
  case 0:
    t__initialize_tests(tests, 4);
    t__mask_uart(NULL, 0, NULL, 0);
    t__analytics[RESCHED_REF].precall = (uint32 (*)())validate_resched;
    t__analytics[DELEG_CLK_REF].precall = (uint32 (*)())no_clock;
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())setup_general;
    queue_table[1].qnext = (queue_t*)0x5;
    queue_table[5].qprev = (queue_t*)0x12;
    ready_list.qnext = (queue_t*)0x55;
    ready_list.qprev = (queue_t*)0x83;
    break;
  case 1:
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())setup_enqueue; break;
  case 2:
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())setup_dequeue; break;
  case 3:
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())setup_resched; break;
  }
}
