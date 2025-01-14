#include <barelib.h>
#include <tty.h>
#include "testing.h"

static tests_t tests[] = {
  {
    .category = "General",
    .prompts = {
      "Program Compiles",
      "TTY Initialized"
    }
  },
  {
    .category = "Getc",
    .prompts = {
      "Thread blocks when no char available",
      "IN buffer count modified",
      "IN buffer head modified",
      "Returns character from IN buffer"
    }
  },
  {
    .category = "Putc",
    .prompts = {
      "Thread blocks when buffer is full",
      "Character added to the OUT buffer",
      "OUT buffer count modified",
      "UART interrupt enabled"
    }
  },
  {
    .category = "RX Handling",
    .prompts = {
      "Character added to buffer",
      "RX handler modifies in buffer count",
      "Character is forwarded to tty_out",
      "Enables UART interrupt on output",
      "Semaphore posts for getc on newline"
    }
  },
  {
    .category = "TX Handling",
    .prompts = {
      "TX handler send character to device",
      "TX handler modifies out buffer counter",
      "TX handler modifies out buffer head",
      "Disables UART interrupt when empty",
      "Semaphore posts for putc"
    }
  }
};


/* Note - Use a uart_emulator variable to shadow the `uart` variable to spoof read/writes */
extern volatile byte* uart;
void uart_handler(void);

byte volatile uart_proxy[5], *uart_old;
static uint32 disabled(void) {
  return 1;
}

static int phase = 2;
static uint32 pre_uart_interrupt_enable(byte enabled) {
  assert(enabled != 0, tests[phase].results[3], "s", "FAIL - UART interrupt not set to enabled");
  return 1;
}

static uint32 pre_uart_interrupt_disable(byte enabled) {
  assert(enabled == 0, tests[4].results[3], "s", "FAIL - UART interrupt set to enabled when inside `uart_handler`");
  return 1;
}

static uint32 pre_post(int32* result, semaphore_t* sem) {
  *result = 0;
  return 1;
}

static uint32 pre_wait(int32* result) {
  tty_in.count = 1;
  tty_in.head = 0;
  tty_in.buffer[0] = 'v';
  *result = 0;
  return 1;
}

static uint32 pre_wait_2(int32* result) {
  tty_in.count = 20;
  tty_in.head = TTY_BUFFLEN - 1;
  tty_in.buffer[TTY_BUFFLEN - 1] = 'x';
  *result = 0;
  return 1;
}

#define DATA_IDX 0
#define ICFG_IDX 1
#define INTR_IDX 2
#define RX_INTR 0x4
#define TX_INTR 0x2
static void validate_uart_interrupt(byte expect, char* error) {
  expect <<= 1;
  assert(uart[ICFG_IDX] == expect, tests[phase].results[3], "s", error);
}

static void run_tx_test(char ch, int32 offset, int32 count) {
  tty_out.count = count + 1;
  tty_out.head = offset;
  tty_out.buffer[offset] = ch;
  uart[ICFG_IDX] = TX_INTR;
  uart[INTR_IDX] = TX_INTR;
  t__reset_analytics();
  uart_handler();

  assert(uart[DATA_IDX] == ch, tests[4].results[0], "s", "FAIL - Character was not written to uart register");
  assert(tty_out.count == count, tests[4].results[1], "s", "FAIL - Count was not decremented during uart handling");
  assert(tty_out.head == (offset + 1) % TTY_BUFFLEN, tests[4].results[2], "s", "FAIL - Head was not adjusted during uart handling");
  assert(t__analytics[SEM_POST_REF].numcalls > 0, tests[4].results[4], "s", "FAIL - Semaphore was not posted during uart handling");
  assert(t__analytics[SEM_POST_REF].numcalls < 2, tests[4].results[4],
	 "s", "FAIL - Semaphore posted too many times during uart handling");
  if (count == 0)
    validate_uart_interrupt(0, "FAIL - UART interrupts enabled when no characters remain");
  else
    validate_uart_interrupt(1, "FAIL - UART interrupts were disabled when characters remained");
}

static void run_rx_test(char ch, int32 in_offset, int32 in_count, int32 out_offset, int32 out_count) {
  int32 in_idx = (in_count + in_offset) % TTY_BUFFLEN;
  int32 out_idx = (out_count + out_offset) % TTY_BUFFLEN;
  uart[DATA_IDX] = ch;
  uart[ICFG_IDX] = 0;
  uart[INTR_IDX] = RX_INTR;
  tty_in.count = in_count;
  tty_in.head = in_offset;
  tty_out.head = out_offset;
  tty_out.count = out_count;

  t__reset_analytics();
  uart_handler();

  assert(tty_in.count == in_count+1, tests[3].results[1], "s", "FAIL - Count was not modified when RX interrupt occurs");
  assert(tty_in.buffer[in_idx] == ch, tests[3].results[0], "s", "FAIL - Character not found at the tail of the buffer with count == 1");
  assert(tty_out.buffer[out_idx] == ch, tests[3].results[2], "s", "FAIL - Character not found at tail index of buffer");
  assert(tty_out.count == out_count+1, tests[3].results[2], "s", "FAIL - Out buffer count was not correct");
  if (ch == '\n') {
    assert(t__analytics[SEM_POST_REF].numcalls > in_count, tests[3].results[4], "FAIL - Semaphore not posted enough times after '\\n'");
    assert(t__analytics[SEM_POST_REF].numcalls < in_count + 2, tests[3].results[4], "FAIL - Semaphore posted too many times after '\\n'");
  }
  else
    assert(t__analytics[SEM_POST_REF].numcalls == 0, tests[3].results[4], "FAIL - `post_sem` called on non-newline character");
  validate_uart_interrupt(1, "FAIL - UART interrupts were disabled, possibly no character mirror");
}

static uint32 setup_general(void) {
  assert(1, tests[0].results[0], "");
  assert(tty_in.sem.state == S_USED, tests[0].results[1], "s", "FAIL - Semaphore was not initialized for tty_in");
  assert(tty_out.sem.state == S_USED, tests[0].results[1], "s", "FAIL - Semaphore was not initialized for tty_out");
  assert(tty_in.sem.queue.key == 0, tests[0].results[1], "s", "FAIL - tty_in semaphore was not set to 0");
  assert(tty_out.sem.queue.key == TTY_BUFFLEN, tests[0].results[1],
	 "s", "FAIL - tty_out semaphore was not set to the size of the buffer");
  assert(tty_in.count == 0, tests[0].results[1], "s", "FAIL - tty_in count was not set to zero");
  assert(tty_out.count == 0, tests[0].results[1], "s", "FAIL - tty_out count was not set to zero");
  t__reset();
  return 1;
}

static uint32 setup_getc(void) {
  char ch;
  
  // Blocking thread
  t__analytics[SEM_WAIT_REF].precall = (uint32 (*)())pre_wait;
  t__reset_analytics();
  
  ch = tty_getc();
  
  assert(ch == 'v', tests[1].results[3], "s", "FAIL - Returned character does not match expected value");
  assert(tty_in.count == 0, tests[1].results[1], "s", "FAIL - Count was not properly decremented");
  assert(tty_in.head == 1, tests[1].results[2], "s", "FAIL - Head was not properly incremented");
  assert(t__analytics[SEM_WAIT_REF].numcalls == 1, tests[1].results[0], "s", "FAIL - Wait was not called on semaphore");

  // Assert Wrap
  t__analytics[SEM_WAIT_REF].precall = (uint32 (*)())pre_wait_2;
  t__reset_analytics();

  ch = tty_getc();
  assert(ch == 'x', tests[1].results[3], "s", "FAIL - Returned character does not match expected value");
  assert(tty_in.count == 19, tests[1].results[1], "s", "FAIL - Count was not properly decremented");
  assert(tty_in.head == 0, tests[1].results[2], "s", "FAIL - Head did not correctly wrap around to start of buffer");
  assert(t__analytics[SEM_WAIT_REF].numcalls == 1, tests[1].results[0], "s", "FAIL - Wait was not called on semaphore");

  t__reset();
  return 1;
}

static uint32 setup_putc(void) {
  t__analytics[SET_UART_INTERRUPT_REF].precall = (uint32 (*)())pre_uart_interrupt_enable;
  t__analytics[SEM_WAIT_REF].precall = (uint32 (*)())disabled;

  // Blocking thread && Basic Read [0]
  tty_out.count = 0;
  tty_out.head = 0;
  tty_out.buffer[0] = '\0';
  t__reset_analytics();
  tty_putc('a');

  assert(tty_out.buffer[0] == 'a', tests[2].results[1], "s", "FAIL - Character not found at correct index in buffer");
  assert(tty_out.count == 1, tests[2].results[2], "s", "FAIL - Count was not incremented when character added to buffer");
  assert(t__analytics[SET_UART_INTERRUPT_REF].numcalls == 1, tests[2].results[3], "s", "FAIL - UART interrupt was not configured");
  assert(t__analytics[SEM_WAIT_REF].numcalls == 1, tests[2].results[0], "s", "FAIL - Semaphore was not awaited");


  tty_out.count = 5;
  tty_out.head = 2;
  tty_out.buffer[7] = '\0';
  t__reset_analytics();
  tty_putc('b');
  assert(tty_out.buffer[7] == 'b', tests[2].results[1],
	 "s", "FAIL - Character not found at correct index when buffer contains characters already");
  assert(tty_out.count == 6, tests[2].results[2], "s", "FAIL - Count was not incremented when character added to buffer");

  // Assert Wrap [TTY_BUFFLEN - 1]
  tty_out.count = TTY_BUFFLEN - 1;
  tty_out.head = 1;
  tty_out.buffer[0] = '\0';
  t__reset_analytics();
  tty_putc('z');
  assert(tty_out.buffer[0] == 'z', tests[2].results[1], "s", "FAIL - Buffer does not properly wrap back to beginning");

  t__reset();
  return 1;
}

static uint32 setup_rx(void) {
  phase = 3;
  t__analytics[SET_UART_INTERRUPT_REF].precall = (uint32 (*)())pre_uart_interrupt_enable;
  t__analytics[SEM_POST_REF].precall = (uint32 (*)())pre_post;
  uart_old = uart;
  uart = uart_proxy;

  // Test simple RX
  run_rx_test('x', 0, 0, 0, 0);

  // Test offset RX
  run_rx_test('l', 2, 5, 0, 0);

  // Test out offset RX
  run_rx_test(';', 0, 0, 8, 1);

  // Test overflow RX
  run_rx_test('a', 10, TTY_BUFFLEN - 2, 0, 0);

  // Test out overflow RX
  run_rx_test(' ', 0, 0, 4, TTY_BUFFLEN - 2);

  // Test newline post
  run_rx_test('\n', 3, 5, 0, 0);

  uart = uart_old;
  t__reset();
  return 1;
}

static uint32 setup_tx(void) {
  phase = 4;
  t__analytics[SET_UART_INTERRUPT_REF].precall = (uint32 (*)())pre_uart_interrupt_disable;
  t__analytics[SEM_POST_REF].precall = (uint32 (*)())pre_post;
  uart_old = uart;
  uart = uart_proxy;

  // Test basic in
  run_tx_test('&', 0, 0);

  // Test offset
  run_tx_test('a', 5, 0);

  // Overflow
  run_tx_test('o', TTY_BUFFLEN - 1, 0);
  
  // Test queue
  run_tx_test('1', 0, 5);

  
  uart = uart_proxy;
  uart = uart_old;
  t__reset();
  return 1;
}

void t__ms8(uint32 run) {
  switch (run) {
  case 0:
    t__initialize_tests(tests, 5);
    t__analytics[RESCHED_REF].precall = (uint32 (*)())disabled;
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())setup_general;
    break;
  case 1:
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())setup_getc; break;
  case 2:
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())setup_putc; break;
  case 3:
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())setup_rx; break;
  case 4:
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())setup_tx; break;

  }
}
