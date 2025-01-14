#include <barelib.h>
#include <bareio.h>
#include "testing.h"
#include "proxy.h"


byte builtin_echo(char*);
byte builtin_hello(char*);
byte shell(char*);

static tests_t tests[] = {
  {
    .category = "General",
    .prompts = {
      "Program Compiles",
      "echo is callable",
      "hello is callable",
      "shell is callable"
    }
  },
  {
    .category = "Echo",
    .prompts = {
      "Prints argument text",
      "Echoes a line of text when interactive",
      "Returns on empty line",
      "Returns 0 with argument text",
      "Returns number of characters when interactive"
    }
  },
  {
    .category = "Hello",
    .prompts = {
      "Prints error when no argument received",
      "Prints correct argument text",
      "Returns 1 on error",
      "Returns 0 on success"
    }
  },
  {
    .category = "Shell",
    .prompts = {
      "Prints initial prompt",
      "Prints error on bad command",
      "Successfully calls echo",
      "Successfully calls hello",
      "Shell loops to run another command",
      "Shell performs $? result replacement"
    }
  }
};


static int strlen(const char* str) {
  int i = 0;
  while (str[i] != '\0') i++;
  return i;
}

static uint32 pre_echo(byte* result, char* arg) {
  assert(1, tests[3].results[2], "");
  *result = 100;
  return 1;
}

static uint32 pre_hello(byte* result, char* arg) {
  assert(1, tests[3].results[3], "");
  *result = 200;
  return 1;
}

static byte post_echo(byte* result, char* arg) {
  assert(1, tests[0].results[1], "");
  return *result;
}

static byte post_hello(byte* result, char* arg) {
  assert(1, tests[0].results[2], "");
  return *result;
}

static char* putc_expects;
static uint32 putc_test, putc_expect_len, putc_idx, category;
static void config_test(int32 cat, int32 test, char* str) {
  putc_expects = str;
  category = cat;
  putc_test = test;
  putc_expect_len = strlen(str);
  putc_idx = 0;
}

static uint32 pre_putc(char* result, char ch) {
  *result = ch;

  if (!assert(putc_idx < putc_expect_len, tests[category].results[putc_test], "sds", "FAIL - uart_putc called with unexpected value after string [", ch, "]"))
    return 1;
  assert(ch == putc_expects[putc_idx++], tests[category].results[putc_test], "s", "FAIL - Printed character did not match expected string");
  return 1;
}

static uint32 pre_getc_echo2(char* result) {
  static byte called = 0;
  *result = '\n';
  assert(putc_idx >= putc_expect_len, tests[1].results[1], "s", "FAIL - Printed less than the expected number of characters");
  config_test(1, 1, "\n");
  assert(called++ == 0, tests[1].results[2], "s", "FAIL - uart_getc called after '\\n\\n' sequence");
  return 1;
}

static uint32 pre_getc_echo1(char* result) {
  static char* input = "some randomish input text\n";
  static uint32 idx = 0;
  int len = strlen(input);

  if (!assert(idx < len, tests[1].results[1], "s", "FAIL - Called uart_getc after line completed"))
    return 1;
  if (idx == len - 1) {
    config_test(1, 1, input);
    t__analytics[UART_GETC_REF].precall = (uint32 (*)())pre_getc_echo2;
  }
  *result = input[idx++];
  return 1;
}

static uint32 pre_getc_shell5(char* result) {
  t__reset();
  return 1;
}

static uint32 pre_getc_shell4(char* result) {
  static char* input = "echo some $? value\n";
  static uint32 idx = 0;

  if (input[idx] == '\n') {
    t__analytics[UART_GETC_REF].precall = (uint32 (*)())pre_getc_shell5;
    t__analytics[ECHO_REF].precall = NULL;
    assert(putc_idx >= putc_expect_len, tests[3].results[5], "s", "FAIL - Printed less than expected number of characters");
    config_test(3, 5, "some 100 value\nbareOS$ ");
  }
  *result = input[idx++];
  return 1;
}

static uint32 pre_getc_shell3(char* result) {
  static char* input = "echo test\n";
  static uint32 idx = 0;

  assert(1, tests[3].results[4], "");
  if (input[idx] == '\n') {
    t__analytics[UART_GETC_REF].precall = (uint32 (*)())pre_getc_shell4;
    assert(putc_idx >= putc_expect_len, tests[3].results[3], "s", "FAIL - Printed less than expected number of characters");
    config_test(3, 2, "bareOS$ ");
  }
  *result = input[idx++];
  return 1;
}

static uint32 pre_getc_shell2(char* result) {
  static char* input = "hello me\n";
  static uint32 idx = 0;

  if (input[idx] == '\n') {
    t__analytics[UART_GETC_REF].precall = (uint32 (*)())pre_getc_shell3;
    assert(putc_idx >= putc_expect_len, tests[3].results[1], "s", "FAIL - Printed less than expected number of characters");
    config_test(3, 3, "bareOS$ ");
  }
  *result = input[idx++];
  return 1;
}

static uint32 pre_getc_shell1(char* result) {
  static char* input = "bad command\n";
  static uint32 idx = 0;

  assert(1, tests[0].results[3], "");
  if (input[idx] == '\n') {
    t__analytics[UART_GETC_REF].precall = (uint32 (*)())pre_getc_shell2;
    assert(putc_idx >= putc_expect_len, tests[3].results[0], "s", "FAIL - Printed less than expected number of characters");
    config_test(3, 1, "Unknown command\nbareOS$ ");
  }
  *result = input[idx++];
  return 1;
}

static uint32 run_general(void) {
  t__analytics[HELLO_REF].postcall = (uint32 (*)())post_hello;
  t__analytics[ECHO_REF].postcall = (uint32 (*)())post_echo;
  
  assert(1, tests[0].results[0], "");
  
  builtin_hello("hello test string");
  builtin_echo("echo echo this text");
  t__reset();
  return 1;
}

static uint32 run_echo(void) {
  t__analytics[UART_PUTC_REF].precall = (uint32 (*)())pre_putc;
  int result;
  
  config_test(1, 0, "test string one\n");
  result = builtin_echo("echo test string one");
  assert(putc_idx >= putc_expect_len, tests[1].results[0], "s", "FAIL - Printed less than the expected number of characters");
  assert(result == 0, tests[1].results[3], "sd", "FAIL - Result of echo was ", result);

  t__analytics[UART_GETC_REF].precall = (uint32 (*)())pre_getc_echo1;
  config_test(1, 1, "\n");
  result = builtin_echo("echo");
  assert(1, tests[1].results[3], "");
  assert(result > 24, tests[1].results[4], "s", "FAIL - Result was less than the number of characters");
  assert(result < 26, tests[1].results[4], "s", "FAIL - Result was greater than the number of characters");
  
  t__reset();
  return 1;
}

static uint32 run_hello(void) {
  int result;

  config_test(2, 0, "Error - bad argument\n");
  result = builtin_hello("hello");
  assert(result == 1, tests[2].results[2], "sds", "FAIL - Returned ", result, " instead of 1");
  
  config_test(2, 1, "Hello, a test example!\n");
  result = builtin_hello("hello a test example");
  assert(result == 0, tests[2].results[3], "sds", "FAIL - Returned ", result, " instead of 0");
  
  t__reset();
  return 1;
}

static uint32 setup_shell(byte* result, char* arg) {
  config_test(3, 0, "bareOS$ ");
  t__analytics[UART_PUTC_REF].precall = (uint32 (*)())pre_putc;
  t__analytics[UART_GETC_REF].precall = (uint32 (*)())pre_getc_shell1;
  t__analytics[HELLO_REF].precall = (uint32 (*)())pre_hello;
  t__analytics[ECHO_REF].precall = (uint32 (*)())pre_echo;

  return 0;
}

void t__ms2(uint32 run) {
  switch (run) {
  case 0:
    t__initialize_tests(tests, 4);
    t__mask_uart(NULL, 0, NULL, 0);
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())run_general;
    break;
  case 1:
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())run_echo;
    break;
  case 2:
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())run_hello;
    break;
  case 3:
    t__mask_uart(NULL, 0, NULL, 0);
    t__analytics[DISP_KERN_REF].precall = NULL;
    t__analytics[SHELL_REF].precall = (uint32 (*)())setup_shell;
    break;
  }
}
