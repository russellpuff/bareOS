#include <barelib.h>
#include <bareio.h>
#include <thread.h>
#include "testing.h"

void resched(void);

static tests_t tests[] = {
  {
    .category = "General",
    .prompts = {
      "Program Compiles"
    }
  },
  {
    .category = "Suspend",
    .prompts = {
      "Threads can suspend",
      "Invalid threads fail to suspend safely",
      "Suspend raises a resched system call"
    }
  },
  {
    .category = "Resume",
    .prompts = {
      "Threads can be readied",
      "Invalid threads fail resume safely",
      "Resume raises a resched system call"
    }
  },
  {
    .category = "Join",
    .prompts = {
      "Frees awaited thread",
      "Returns thread's result",
      "Blocks until thread is defunct"
    }
  },
  {
    .category = "Shell",
    .prompts = {
      "Calls create_thread for all threads",
      "Calls resume_thread for all threads",
      "Calls join_thread for all threads"
    }
  },
  {
    .category = "Resched",
    .prompts = {
      "Alternates between two threads",
      "Cycles between three threads",
      "Readies old thread",
      "Resumes new thread",
      "Timer tick invokes resched"
    }
  }
};

static uint32 no_clock(void) {
  return 1;
}

static int32 expected_state = -1;
static int32 phase = 0;
static uint32 validate_resched(int32* result) {
  char* state_str;
  switch (expected_state) {
  case TH_FREE: state_str = "free"; break;
  case TH_RUNNING: state_str = "running"; break;
  case TH_READY: state_str = "ready"; break;
  case TH_SUSPEND: state_str = "suspended"; break;
  case TH_DEFUNCT: state_str = "defunct"; break;
  }
  if (expected_state != -1)
    assert(thread_table[1].state == expected_state, tests[phase].results[0], "sss", "FAIL - thread was not marked as ", state_str, " on resched");
  *result = current_thread;
  return 1;
}

static uint32 join_resched(int32* result) {
  static int32 pass = 0;
  assert(thread_table[1].state == TH_READY, tests[3].results[2], "s", "FAIL - thread was set to an incorrect state before resched");
  assert(thread_table[1].state != TH_DEFUNCT, tests[3].results[2], "s", "FAIL - thread was set to defunct by join");
  assert(thread_table[1].state != TH_FREE, tests[3].results[2], "s", "FAIL - thread was freed prematurely");
  if (pass++ > 1) {
    thread_table[1].state = TH_DEFUNCT;
  }
  *result = current_thread;
  return 1;
}

static uint32 timer_resched(int32* result) {
  assert(1, tests[5].results[4], "");
  t__reset();
  return 1;
}

static uint32 shell_resched(int32* result) {
  for (int i=1; i<3; i++) {
    if (thread_table[(current_thread + i) % 3].state == TH_READY) {
      int old = current_thread;
      current_thread = (current_thread + i) % 3;
      thread_table[old].state = (thread_table[old].state == TH_RUNNING ? TH_READY : thread_table[old].state);
      thread_table[current_thread].state = TH_RUNNING;
      ctxsw(&thread_table[current_thread].stackptr, &thread_table[old].stackptr);
      return 0;
    }
  }
  return 0;
}

static char* app;
static uint32 pre_putc(char* result, char ch) {
  assert(t__analytics[CREATE_THREAD_REF].numcalls == 1, tests[4].results[0], "sss", "FAIL - ", app, " thread was not created");
  assert(t__analytics[RESUME_THREAD_REF].numcalls == 1, tests[4].results[1], "sss", "FAIL - ", app, " thread was not resumed");
  if (app[0] == 'h')
    assert(t__analytics[JOIN_THREAD_REF].numcalls == 1, tests[4].results[2], "sss", "FAIL - ", app, " thread was not joined");
  else if (app[0] == 'e')
    assert(t__analytics[JOIN_THREAD_REF].numcalls == 1, tests[4].results[2], "sss", "FAIL - ", app, " thread was not joined");
  *result = ch;
  return 1;
}

static char* input = "echo me\nhello world\n";
static uint32 pre_getc(char* result) {
  static int32 idx = 0;
  if (idx >= 20)
    t__reset();
  t__reset_analytics();
  *result = input[idx++];
  return 1;
}

static uint32 pre_shell(int32* result, char* arg) {
  app = "shell";
  t__analytics[UART_GETC_REF].precall = (uint32 (*)())pre_getc;
  t__analytics[UART_PUTC_REF].precall = (uint32 (*)())pre_putc;
  return 0;
}

static uint32 pre_echo(int32* result, char* arg) {
  app = "echo";
  *result = 0;
  return 1;
}

static uint32 pre_hello(int* result, char* arg) {
  app = "hello";
  *result = 0;
  return 1;
}

static uint32 expected_new_thread;
static uint32 expected_old_thread;
static uint32 thread_count;
static void setup_ctxsw(uint32 new, uint32 old, uint32 count) {
  expected_new_thread = new;
  expected_old_thread = old;
  thread_count = count;
}

static uint32 pre_ctxsw(uint64** new, uint64** old) {
  uint32 test = (thread_count == 3 ? 1 : 0);
  assert(new == &thread_table[expected_new_thread].stackptr, tests[5].results[test], "s", "FAIL - Incorrect new thread passed to ctxsw");
  assert(old == &thread_table[expected_old_thread].stackptr, tests[5].results[test], "s", "FAIL - Incorrect old thread passed to ctxsw");
  assert(current_thread == expected_new_thread, tests[5].results[test], "s", "FAIL - Current thread not set before ctxsw");
  if (thread_count == 4)
    assert(thread_table[expected_old_thread].state == TH_SUSPEND, tests[5].results[2], "s", "FAIL - Old thread was readied when it was previously suspended");
  else
    assert(thread_table[expected_old_thread].state == TH_READY, tests[5].results[2], "s", "FAIL - Old thread not set to ready before ctxsw");
  assert(thread_table[expected_new_thread].state == TH_RUNNING, tests[5].results[3], "s", "FAIL - New thread not set to running before ctxsw");
  

  return 1;
}

static uint32 setup_general(void) {
  assert(1, tests[0].results[0], "");
  t__reset();
  return 0;
}

static uint32 setup_suspend(void) {
  int result;
  phase = 1;

  thread_table[1].state = TH_FREE;
  t__reset_analytics();
  result = suspend_thread(1);
  assert(result == -1, tests[1].results[1], "s", "FAIL - Thread does not return proper value for TH_FREE thread");
  assert(t__analytics[RESCHED_REF].numcalls == 0, tests[1].results[1], "s", "FAIL - resched called on invalid thread");

  t__reset_analytics();	
  thread_table[1].state = TH_DEFUNCT;
  result = suspend_thread(1);
  assert(result == -1, tests[1].results[1], "s", "FAIL - Thread does not return proper value for TH_DEFUNCT thread");
  assert(t__analytics[RESCHED_REF].numcalls == 0, tests[1].results[1], "s", "FAIL - resched called on invalid thread");

  t__reset_analytics();
  result = suspend_thread(NTHREADS);
  assert(result == -1, tests[1].results[1], "s", "FAIL - Thread does not return proper value for out of bound thread id");
  assert(t__analytics[RESCHED_REF].numcalls == 0, tests[1].results[1], "s", "FAIL - resched called on invalid thread");

  t__reset_analytics();
  expected_state = TH_SUSPEND;
  thread_table[1].state = TH_READY;
  result = suspend_thread(1);
  expected_state = -1;
  assert(result == 1, tests[1].results[0], "s", "FAIL - Returned value does not match expected thread id");
  assert(t__analytics[RESCHED_REF].numcalls > 0, tests[1].results[2], "s", "FAIL - resched was not called");
  assert(t__analytics[RESCHED_REF].numcalls < 2, tests[1].results[2], "s", "FAIL - resched called too many times");

  t__reset();
  return 0;
}

static uint32 setup_resume(void) {
  int result;
  phase = 2;
  
  thread_table[1].state = TH_FREE;
  t__reset_analytics();
  result = resume_thread(1);
  assert(result == -1, tests[2].results[1], "s", "FAIL - Thread does not return proper value for TH_FREE thread");
  assert(t__analytics[RESCHED_REF].numcalls == 0, tests[2].results[1], "s", "FAIL - resched called on invalid thread");

  t__reset_analytics();
  thread_table[1].state = TH_DEFUNCT;
  result = resume_thread(1);
  assert(result == -1, tests[2].results[1], "s", "FAIL - Thread does not return proper value for TH_DEFUNCT thread");
  assert(t__analytics[RESCHED_REF].numcalls == 0, tests[2].results[1], "s", "FAIL - resched called on invalid thread");

  t__reset_analytics();
  result = resume_thread(NTHREADS);
  assert(result == -1, tests[2].results[1], "s", "FAIL - Thread does not return proper value for out of bound thread id");
  assert(t__analytics[RESCHED_REF].numcalls == 0, tests[2].results[1], "s", "FAIL - resched called on invalid thread");

  t__reset_analytics();
  expected_state = TH_READY;
  thread_table[1].state = TH_SUSPEND;
  result = resume_thread(1);
  expected_state = -1;
  assert(result == 1, tests[2].results[0], "s", "FAIL - Returned value does not match expected thread id");
  assert(t__analytics[RESCHED_REF].numcalls > 0, tests[2].results[2], "s", "FAIL - resched was not called");
  assert(t__analytics[RESCHED_REF].numcalls < 2, tests[2].results[2], "s", "FAIL - resched called too many times");

  t__reset();
  return 0;
}

static uint32 setup_join(void) {
  int result;
  phase = 3;

  t__reset_analytics();
  thread_table[1].state = TH_DEFUNCT;
  thread_table[1].retval = 5;
  result = join_thread(1);
  assert(t__analytics[RESCHED_REF].numcalls == 0, tests[3].results[2], "s", "FAIL - resched called on thread already in defunct state");
  assert(result == 5, tests[3].results[1], "s", "FAIL - returned value does not match thread's return value");
  assert(thread_table[1].state == TH_FREE, tests[3].results[0], "s", "FAIL - thread was not freed after completion");

  t__reset_analytics();
  t__analytics[RESCHED_REF].precall = (uint32 (*)())join_resched;
  thread_table[1].state = TH_READY;
  result = join_thread(1);
  assert(t__analytics[RESCHED_REF].numcalls > 2, tests[3].results[2], "s", "FAIL - resched was not called until thread was defunct");
  assert(t__analytics[RESCHED_REF].numcalls < 4, tests[3].results[2], "s", "FAIL - resched called too many times");
  t__analytics[RESCHED_REF].precall = (uint32 (*)())validate_resched;


  t__reset_analytics();
  result = join_thread(NTHREADS);
  assert(result == -1, tests[3].results[1], "s", "FAIL - Thread does not return proper value for out of bound thread id");
  assert(t__analytics[RESCHED_REF].numcalls == 0, tests[3].results[1], "s", "FAIL - resched called on invalid thread");
  
  t__reset();
  return 0;
}

static uint32 setup_shell(void) {
  t__mask_uart(NULL, 0, NULL, 0);
  thread_table[1].state = TH_FREE;
  t__analytics[RESCHED_REF].precall = (uint32 (*)())shell_resched;
  t__analytics[SHELL_REF].precall = (uint32 (*)())pre_shell;
  t__analytics[ECHO_REF].precall = (uint32 (*)())pre_echo;
  t__analytics[HELLO_REF].precall = (uint32 (*)())pre_hello;
  
  return 0;
}

static uint32 setup_resched(void) {
  t__analytics[RESCHED_REF].precall = NULL;
  t__analytics[CTXSW_REF].precall = (uint32 (*)())pre_ctxsw;
  // setup_ctx(new, old, count);

  // Create 2 threads
  thread_table[0].state = TH_RUNNING;
  thread_table[1].state = TH_READY;
  
  // Call resched
  setup_ctxsw(1, 0, 2);
  resched();

  // Call resched
  setup_ctxsw(0, 1, 2);
  resched();

  // Create 3rd thread
  thread_table[0].state = TH_RUNNING;
  thread_table[1].state = TH_READY;
  thread_table[2].state = TH_READY;
  current_thread = 0;
  
  // Call resched
  setup_ctxsw(1, 0, 3);
  resched();

  // Call resched
  setup_ctxsw(2, 1, 3);
  resched();

  // Call resched
  setup_ctxsw(0, 2, 3);
  resched();


  // Reset threads to 0:SUSPEND,1:READY
  thread_table[0].state = TH_SUSPEND;
  thread_table[1].state = TH_READY;
  thread_table[2].state = TH_FREE;
  current_thread = 0;
  
  // Call resched
  setup_ctxsw(1, 0, 4);
  resched();
  
  // Reset threads to -2:RUNNING,-1:SUSPEND,4:READY
  thread_table[0].state = TH_FREE;
  thread_table[1].state = TH_FREE;
  thread_table[2].state = TH_FREE;
  thread_table[4].state = TH_READY;
  thread_table[7].state = TH_RUNNING;
  thread_table[8].state = TH_SUSPEND;
  thread_table[9].state = TH_DEFUNCT;
  current_thread = 7;
  
  // Call resched
  setup_ctxsw(4, 7, 3);
  resched();

  // Set resched to test for call
  t__analytics[RESCHED_REF].precall = (uint32 (*)())timer_resched;
  t__analytics[DELEG_CLK_REF].precall = NULL;
  while (1) {

  }
  
  t__reset();
  return 0;
}

void t__ms3(uint32 run) {
  switch (run) {
  case 0:
    t__initialize_tests(tests, 6);
    t__mask_uart(NULL, 0, NULL, 0);
    t__analytics[RESCHED_REF].precall = (uint32 (*)())validate_resched;
    t__analytics[DELEG_CLK_REF].precall = (uint32 (*)())no_clock;
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())setup_general; break;
  case 1:
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())setup_suspend; break;
  case 2:
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())setup_resume; break;
  case 3:
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())setup_join; break;
  case 4:
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())setup_shell; break;
  case 5:
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())setup_resched; break;
  }
}
