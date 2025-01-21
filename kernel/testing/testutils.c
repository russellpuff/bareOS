#include <barelib.h>
#include "testing.h"
#include "complete.h"
#include "proxy.h"

#define SYSCON_ADDR 0x100000

static tests_t* t__tests;
static unsigned int ncategories = 0;

static int strlen(const char* str) {
  int i = 0;
  while (str[i] != '\0') i++;
  return i;
}


char __real_uart_putc(char);
static void t__puts(const char* str, int term) {
  while (*str != '\0') __real_uart_putc(*str++);
  if (term) __real_uart_putc('\n');
}

static void t__puti(int val, int base) {
  int i = 0, v, buff[9];
  if (base == 10 && val < 0) {
    v = 0 - val;
    __real_uart_putc('-');
  }
  else {
    v = (unsigned int)val;
  }
  for (; v > 0; v = v / base) buff[i++] = v % base;
  for (i = i - 1; i >= 0; i--)
    __real_uart_putc(buff[i] + (buff[i] < 10 ? '0' : 'a'));
}

static int t__putss(char* dst, int len, const char* src) {
  int i = 0;
  for (; src[i] != '\0' && --len > 0; i++)
    dst[i] = src[i];
  return i;
}

static int t__putis(char* dst, int len, int val, int base) {
  unsigned int i = 0, v, buff[9];
  if (base == 10 && val < 0 && len) {
    v = 0 - val;
    len -= 1;
    *dst++ = '-';
  }
  else {
    v = (unsigned int)val;
  }
  for (; v > 0; v = v / base)
    buff[i++] = v % base;
  for (int w = i - 1; w >= 0 && --len > 0; w--)
    *dst++ = buff[w] + (buff[w] < 10 ? '0' : ('a' - 10));
  return i;
}

static char* __build_result(char* buf, int len, const char* fmt, va_list args) {
  char* base = buf;
  len -= 1;
  for (int i=0; i<len; i++)
    buf[i] = '\0';

  while (*fmt != '\0') {
    switch (*fmt) {
    case 's':
      buf += t__putss(buf, len - (buf - base), (char*)va_arg(args, char*)); break;
    case 'd':
      buf += t__putis(buf, len - (buf - base), (int)va_arg(args, int), 10); break;
    case 'c':
      *buf = (char)va_arg(args, int);
      buf += 1; break;
    case 'x':
      buf += t__putss(buf, len - (buf - base), "0x");
      buf += t__putis(buf, len - (buf - base), (int)va_arg(args, int), 16); break;
    }
    fmt += 1;
  }
  return base;
}


char* t__build_result(char* buf, int len, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  __build_result(buf, len, fmt, args);
  va_end(args);
  return buf;
}

byte assert(int32 pred, char* buf, char* template, ...) {
  va_list args;
  
  va_start(args, template);
  if (pred == 0)
    __build_result(buf, T_MAX_ERR_LEN, template, args);
  va_end(args);
  
  if (buf[0] == 'U')
    t__build_result(buf, T_MAX_ERR_LEN, "s", "OK");
  return pred;
}

static thread_t t__thread_table[NTHREADS];
static uint32 t__current_thread;
static queue_t t__queue_table[NTHREADS];
static queue_t t__ready_list;
static queue_t t__sleep_list;
static alloc_t t__heap;
static ring_buffer_t t__tty_in;
static ring_buffer_t t__tty_out;
static filetable_t t__oft[NUM_FD];
static fsystem_t t__fsd;
static byte t__fs_bitmask[1024];

extern alloc_t* freelist;

void shutdown(void) {
  *(uint32*)SYSCON_ADDR = 0x5555;
}

void t__reset(void) {
  int32 raise_syscall(uint32);
  raise_syscall(SYS_RESET);
}

static int run = 0;
void t__setup_testsuite(void) {
  if (ncategories && run >= ncategories) {
    t__print_results();
    shutdown();
  }
  
#if MILESTONE == 1
  void t__ms1(uint32);
  t__ms1(run++);
#elif MILESTONE == 2
  void t__ms2(uint32);
  t__ms2(run++);
#elif MILESTONE == 3
  void t__ms3(uint32);
  t__ms3(run++);
#elif MILESTONE == 4
  void t__ms4(uint32);
  t__ms4(run++);
#elif MILESTONE == 5
  void t__ms5(uint32);
  t__ms5(run++);
#elif MILESTONE == 6
  void t__ms6(uint32);
  t__ms6(run++);
#elif MILESTONE == 7
  void t__ms7(uint32);
  t__ms7(run++);
#elif MILESTONE == 8
  void t__ms8(uint32);
  t__ms8(run++);
#elif MILESTONE == 9
  void t__ms9(uint32);
  t__ms9(run++);
#elif MILESTONE == 10
  void t__ms10(uint32);
  t__ms10(run++);
#endif
}

void t__mem_snapshot(void) {
  t__current_thread = current_thread;
  t__ready_list = ready_list;
  t__sleep_list = sleep_list;
  if (freelist != NULL)
    t__heap = *freelist;

  t__tty_in = tty_in;
  t__tty_out = tty_out;

  for (int i=0; i<NUM_FD; i++) {
    t__oft[i] = oft[i];
  }
  if (fsd != NULL) {
    t__fsd.device = fsd->device;
    t__fsd.freemasksz = fsd->freemasksz;
    t__fsd.root_dir = fsd->root_dir;
    t__fsd.freemask = t__fs_bitmask;
    for (int i=0; i<fsd->freemasksz; i++)
      t__fsd.freemask[i] = fsd->freemask[i];
  }
  
  for (int i=0; i<NTHREADS; i++) {
    t__thread_table[i] = thread_table[i];
    t__queue_table[i] = queue_table[i];
  }
}

void t__mem_reset(int restore, int tid) {
  current_thread = (restore == 0 ? (tid == -1 ? 0 : tid) : t__current_thread);
  ready_list = (restore == 0 ? (queue_t){ .qnext = 0, .qprev = 0, .key = 0 } : t__ready_list);
  sleep_list = (restore == 0 ? (queue_t){ .qnext = 0, .qprev = 0, .key = 0 } : t__sleep_list);
  if (freelist != NULL) {
    if (restore == 0)
      init_heap();
    else
      *freelist = t__heap;
  }

  tty_in = (restore == 0 ? (ring_buffer_t){ 0 } : t__tty_in);
  tty_out = (restore == 0 ? (ring_buffer_t){ 0 } : t__tty_out);

  for (int i=0; i<NUM_FD; i++) {
    oft[i] = (restore == 0 ? (filetable_t){ 0 } : t__oft[i]);
  }
  if (fsd != NULL) {
    fsd->device = (restore == 0 ? (bdev_t){ 0 } : t__fsd.device);
    fsd->freemasksz = (restore == 0 ? 0 : t__fsd.freemasksz);
    fsd->root_dir = (restore == 0 ? (directory_t){ 0 } : t__fsd.root_dir);
    if (fsd->freemask != NULL) {
      for (int i=0; i<fsd->freemasksz; i++)
	fsd->freemask[i] = (restore == 0 ? 0 : t__fsd.freemask[i]);
    }
  }
  
  for (int i=0; i<NTHREADS; i++) {
    thread_table[i] = (restore == 0 ? (thread_t){ .state = TH_FREE } : t__thread_table[i]);
    queue_table[i] = (restore == 0 ? (queue_t){ .qnext = NULL, .qprev = NULL } : t__queue_table[i]);
  }
}

void t__print_results(void) {
  t__unmask_uart();
  for (int i=0; i<ncategories; i++) {
    for (int j=0; t__tests[i].prompts[j] && t__tests[i].prompts[j][0] != '\0'; j++) {
      if (t__tests[i].results[j][0] == 'U')
	t__build_result(t__tests[i].results[j], T_MAX_ERR_LEN, "s", "FAIL - Relevant function was never called, test not performed");
    }
  }
  
  t__puts("\n  [Test Results for unit testing on MILESTONE ", 0);
  t__puti(MILESTONE, 10);
  t__puts("]\n", 1);
  for (int i=0; i<ncategories; i++) {
    int max_len = 0;
    t__puts(t__tests[i].category, 1);
    for (int j=0; j<t__tests[i].ntests; j++) {
      int len = strlen(t__tests[i].prompts[j]) + 2;
      max_len = (max_len > len ? max_len : len);
    }
    for (int j=0; j<max_len+5; j++) t__puts("━", 0);
    t__puts("┳━━┅", 1);
    for (int j=0; j<t__tests[i].ntests; j++) {
      int pad_len = max_len - strlen(t__tests[i].prompts[j]);
      t__puts("    ", 0);
      t__puts(t__tests[i].prompts[j], 0);
      for (int i=0; i<pad_len; i++)
	t__puts(" ", 0);
      t__puts(" │ ", 0);
      t__puts(t__tests[i].results[j], 1);
    }
    t__puts("\n", 0);
  }
}

static char* input_buffer, *output_buffer;
static uint32 input_buffer_len, input_buffer_idx = 0, output_buffer_len, output_buffer_idx = 0;
static uint32 mask_uart_putc(char* result, char ch) {
  if (output_buffer != NULL) {
    output_buffer[output_buffer_idx++] = ch;
    output_buffer_idx %= output_buffer_len;
  }
  *result = ch;
  return 1;
}
static uint32 mask_uart_getc(char* result) {
  if (input_buffer != NULL) {
    *result = input_buffer[input_buffer_idx++];
    input_buffer_idx %= input_buffer_len;
  }
  else {
    *result = -1;
  }
  return 1;
}
void t__mask_uart(char* input, uint32 ilen, char* output, uint32 olen) {
  input_buffer = input;
  input_buffer_len = ilen;
  input_buffer_idx = 0;
  output_buffer = output;
  output_buffer_len = olen;
  output_buffer_idx = 0;
  t__analytics[UART_PUTC_REF].precall = (uint32 (*)())mask_uart_putc;
  t__analytics[UART_GETC_REF].precall = (uint32 (*)())mask_uart_getc;
}

void t__unmask_uart(void) {
  t__analytics[UART_PUTC_REF].precall = NULL;
  t__analytics[UART_GETC_REF].precall = NULL;
}

void t__write_timeout(void) {
  int idx = run - 1;
  t__unmask_uart();
  for (int j=0; t__tests[idx].prompts[j] && t__tests[idx].prompts[j][0] != '\0'; j++) {
    if (t__tests[idx].results[j][0] == 'U')
      t__build_result(t__tests[idx].results[j], T_MAX_ERR_LEN, "s", "TIMEOUT - Test did return after reasonable period");
  }
}

void t__initialize_tests(tests_t* tests, int ncat) {
  ncategories = ncat;
  for (int i=0; i<ncategories; i++) {
    tests[i].ntests = 0;
    for (int j=0; tests[i].prompts[j] && tests[i].prompts[j][0] != '\0'; j++) {
      tests[i].ntests += 1;
      t__build_result(tests[i].results[j], T_MAX_ERR_LEN, "s", "UNTESTED - Tester could not perform test");
    }
  }
  t__tests = tests;
}
