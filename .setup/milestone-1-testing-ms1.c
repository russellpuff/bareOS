#include <barelib.h>
#include <bareio.h>
#include "testing.h"

static tests_t tests[] = {
  {
    .category = "General",
    .prompts = {
      "Program Compiles",
      "kprintf is callable"
    }
  },
  {
    .category = "kprintf Function",
    .prompts = {
      "Calls uart_putc",
      "Text passed to uart_putc",
      "Decimal %d template prints integer",
      "Hexadecimal %x template prints hex",
      "Multiple templates",
      "Correctly handles decimal zero",
      "Correctly handles hexadecimal zero"
    }
  },
  {
    .category = "Initialization",
    .prompts = {
      "Prints correct Text start address",
      "Prints correct Kernal size",
      "Prints correct Data start address",
      "Prints correct free memory start address",
      "Prints correct free memory size"
    }
  }
};

extern byte text_start;    /*                                             */
extern byte data_start;    /*  These variables automatically contain the  */
extern byte bss_start;     /*  lowest  address of important  segments in  */
extern byte mem_start;     /*  memory  created  when  the  kernel  boots  */
extern byte mem_end;       /*    (see mmap.c and kernel.ld for source)    */

static int pre_putc_1(void* result, char ch) {
  return 1;
}

static int expect_len, curr_test, count;
static const char* ref;
static const char* tail;
static int pre_putc_2(void* result, char ch) {
  assert(1, tests[1].results[0], "");
  assert(ref[count] == ch, tests[1].results[curr_test], "ss", "FAIL - Text sent to uart_putc did not match the ", tail);
  count = (count + 1) % expect_len;
  return 1;
}

static char lines[5][1024];
static int pre_putc_3(void* result, char ch) {
  static int line_idx = 0;
  static int offset = 0;
  if (ch == '\n') {
    int i = 0;
    for (; lines[line_idx][i] != '\n'; i++);
    assert(offset < i + 1, tests[2].results[line_idx], "s", "FAIL - Line printed too many characters");
    assert(offset > i - 1, tests[2].results[line_idx], "s", "FAIL - Line printed too few characters");
    line_idx += 1;
    offset = 0;
  }
  else {
    assert(lines[line_idx][offset] == ch, tests[2].results[line_idx], "sd", "FAIL - Character incorrect at position ", offset);
    offset += 1;
  }
  return 1;
}

static int pre_display_1(void* result) {
  assert(1, tests[0].results[0], "");
  kprintf("value");
  assert(1, tests[0].results[1], "");
  t__reset();
  return 1;
}

static int set_config(int l, int t, const char* r, const char* ta) {
  expect_len = l;
  curr_test = t;
  count = 0;
  ref = r;
  tail = ta;
  return l;
}

static int pre_display_2(void* result) {
  int len;
  t__reset_analytics();
  len = set_config(19, 1, "Longer string test\n", "string template");
  kprintf("Longer string test\n");

  assert(t__analytics[UART_PUTC_REF].numcalls == len, tests[1].results[1], "s", "FAIL - uart_putc was not called the correct number of times for the input");

  t__reset_analytics();
  len = set_config(16, 2, "Test: 444 value\n", "decimal template");
  kprintf("Test: %d value\n", 444);

  assert(t__analytics[UART_PUTC_REF].numcalls == len, tests[1].results[2], "s", "FAIL - uart_putc was not called the correct number of times for the input");

  t__reset_analytics();
  len = set_config(19, 3, "Test: 0xab54ca hex\n", "hex template");
  kprintf("Test: %x hex\n", 0xab54ca);

  assert(t__analytics[UART_PUTC_REF].numcalls == len, tests[1].results[3], "s", "FAIL - uart_putc was not called the correct number of times for the input");

  t__reset_analytics();
  len = set_config(17, 4, "Test: 0xab54ca12\n", "mixed template");
  kprintf("Test: %x%d\n", 0xab54ca, 12);

  assert(t__analytics[UART_PUTC_REF].numcalls == len, tests[1].results[4], "s", "FAIL - uart_putc was not called the correct number of times for the input");

  t__reset_analytics();
  len = set_config( 8, 5, "Test: 0\n", "zero template");
  kprintf("Test: %d\n", 0);

  assert(t__analytics[UART_PUTC_REF].numcalls == len, tests[1].results[5], "s", "FAIL - uart_putc was not called the current number of times for the input");

  t__reset_analytics();
  len = set_config( 10, 6, "Test: 0x0\n", "zero template");
  kprintf("Test: %x\n", 0);

  assert(t__analytics[UART_PUTC_REF].numcalls == len, tests[1].results[6], "s", "FAIL - uart_putc was not called the current number of times for the input");
  t__reset();
  return 1;
}

static void endline(char* str) {
  int i=0;
  for (; str[i] != '\0'; i++);
  str[i] = '\n';
}

static int pre_display_3(void* result) {
  endline(t__build_result(lines[0], 1024, "sx", "Kernel start: ", (unsigned long)&text_start));
  endline(t__build_result(lines[1], 1024, "sd", "--Kernel size: ", (unsigned long)(&data_start - &text_start)));
  endline(t__build_result(lines[2], 1024, "sx", "Globals start: ", (unsigned long)&data_start));
  endline(t__build_result(lines[3], 1024, "sx", "Heap/Stack start: ", (unsigned long)&mem_start));
  endline(t__build_result(lines[4], 1024, "sd", "--Free Memory Available: ", (unsigned long)(&mem_end - &mem_start)));

  return 0;
}

static void post_display_3(void) {
  t__reset();
}

void t__ms1(uint32 run) {

  switch (run) {
  case 0:
    t__initialize_tests(tests, 3);
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())pre_display_1;
    t__analytics[UART_PUTC_REF].precall = (uint32 (*)())pre_putc_1;
    break;
  case 1:
    t__analytics[UART_PUTC_REF].precall = (uint32 (*)())pre_putc_2;
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())pre_display_2;
    break;
  case 2:
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())pre_display_3;
    t__analytics[UART_PUTC_REF].precall = (uint32 (*)())pre_putc_3;
    t__analytics[DISP_KERN_REF].postcall = (uint32 (*)())post_display_3;
    break;
  }
}
