#include <barelib.h>
#include "complete.h"
#include "testing.h"
#include "proxy.h"

analytic_t t__analytics[T_MAX_REF] = { 0 };

extern volatile uint32* clint_timer_addr;
static const uint32 testing_interval = 100000;
static uint32 timer;

int32 maybe_precall(uint32 (*)(), void*);

__attribute__((interrupt ("machine")))
void __wrap_delegate_clk(void) {
  uint32 ref = DELEG_CLK_REF;
  *clint_timer_addr += testing_interval;
  if (--timer == 0) {
    t__write_timeout();
    void _start(void);
    _start();
  }
  if (t__analytics[ref].precall &&
      ((uint32 (*)(void))t__analytics[ref].precall)())
      return;
  asm("li t0, 0x20\n"
      "csrs mip, t0\n"
      "csrs mie, t0\n");
}

__attribute__((naked))
void  __wrap_handle_clk(void) {
  asm volatile ("");
  __attribute__((interrupt ("supervisor"))) void __real_handle_clk(void);
  asm ("j __real_handle_clk");
}

extern int32 signum;
s_interrupt __wrap_handle_syscall(void) {
  int32 ref = HANDLE_SYSCALL_REF;
  if (signum == SYS_RESET) {
    void supervisor_start(void);
    supervisor_start();
  }
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(uint32))t__analytics[ref].precall)(signum))
    return;

  /*  Spoof standard syscall behavior */
  extern void (*syscall_table[])(void);
  syscall_table[signum]();
  
  if (t__analytics[ref].postcall)
    ((int32 (*)(uint32))t__analytics[ref].postcall)(signum);
}

void t__reset_analytics(void) {
  for (int i=0; i<T_MAX_REF; i++) t__analytics[i].numcalls = 0;
}

void __wrap_supervisor_start(void) {
  uint32 ref = SUP_START_REF;
  t__analytics[ref].numcalls += 1;

  timer = 100;
  t__setup_testsuite();

  if (t__analytics[ref].precall &&
      ((uint32 (*)())t__analytics[ref].precall)())
    return;

  void __real_supervisor_start(void);
  __real_supervisor_start();
  if (t__analytics[ref].postcall)
    ((void (*)())t__analytics[ref].postcall)();
}

void __wrap_initialize(void) {
  uint32 ref = INIT_REF;
  t__analytics[ref].numcalls += 1;

  if (t__analytics[ref].precall &&
      ((uint32 (*)())t__analytics[ref].precall)())
    return;

  void __real_initialize(void);
  __real_initialize();
  if (t__analytics[ref].postcall)
    ((void (*)())t__analytics[ref].postcall)();
}

void __wrap_display_kernel_info(void) {
  uint32 ref = DISP_KERN_REF;
  t__analytics[ref].numcalls += 1;

  if (t__analytics[ref].precall &&
      ((uint32 (*)())t__analytics[ref].precall)())
    return;

  void __real_display_kernel_info(void);
  __real_display_kernel_info();
  if (t__analytics[ref].postcall)
    ((void (*)())t__analytics[ref].postcall)();
}

byte __wrap_shell(char* arg) {
  byte result, ref = SHELL_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(byte*, char*))t__analytics[ref].precall)(&result, arg))
    return result;

  byte __real_shell(char*);
  result = __real_shell(arg);
  if (t__analytics[ref].postcall)
    return ((byte (*)(byte*, char*))t__analytics[ref].postcall)(&result, arg);
  return result;
}

byte __wrap_builtin_hello(char* arg) {
  byte result, ref = HELLO_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(byte*, char*))t__analytics[ref].precall)(&result, arg))
    return result;

  byte __real_builtin_hello(char*);
  result = __real_builtin_hello(arg);
  if (t__analytics[ref].postcall)
    return ((byte (*)(byte*, char*))t__analytics[ref].postcall)(&result, arg);
  return result;
}

byte __wrap_builtin_echo(char* arg) {
  byte result, ref = ECHO_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(byte*, char*))t__analytics[ref].precall)(&result, arg))
    return result;

  byte __real_builtin_echo(char*);
  result = __real_builtin_echo(arg);
  if (t__analytics[ref].postcall)
    return ((byte (*)(byte*, char*))t__analytics[ref].postcall)(&result, arg);
  return result;
}

int32 __wrap_create_thread(void* addr, char* arg, uint32 len) {
  int32 result, ref = CREATE_THREAD_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(int32*, void*, char*, uint32))t__analytics[ref].precall)(&result, addr, arg, len))
    return result;

  int32 __real_create_thread(void*, char*, uint32);
  result = __real_create_thread(addr, arg, len);
  if (t__analytics[ref].postcall)
    return ((int32 (*)(int32*, void*, char*, uint32))t__analytics[ref].postcall)(&result, addr, arg, len);
  return result;
}

int32 __wrap_kill_thread(uint32 tid) {
  int32 result, ref = KILL_THREAD_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(int32*, uint32))t__analytics[ref].precall)(&result, tid))
    return result;

  int32 __real_kill_thread(uint32);
  result = __real_kill_thread(tid);
  if (t__analytics[ref].postcall)
    return ((int32 (*)(int32*, uint32))t__analytics[ref].postcall)(&result, tid);
  return result;
}

void __wrap_ctxsw(uint32** dst, uint32** src) {
  byte ref = CTXSW_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(uint32**, uint32**))t__analytics[ref].precall)(dst, src))
    return;

  void __real_ctxsw(uint32**, uint32**);
  __real_ctxsw(dst, src);
  if (t__analytics[ref].postcall)
    ((void (*)(uint32**, uint32**))t__analytics[ref].postcall)(dst, src);
}

int32 __wrap_resume_thread(uint32 tid) {
  int32 result, ref = RESUME_THREAD_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(int32*, uint32))t__analytics[ref].precall)(&result, tid))
    return result;

  int32 __real_resume_thread(uint32);
  result = __real_resume_thread(tid);
  if (t__analytics[ref].postcall)
    return ((int32 (*)(int32*, uint32))t__analytics[ref].postcall)(&result, tid);
  return result;
}

int32 __wrap_suspend_thread(uint32 tid) {
  int32 result, ref = SUSPEND_THREAD_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(int32*, uint32))t__analytics[ref].precall)(&result, tid))
    return result;

  int32 __real_suspend_thread(uint32);
  result = __real_suspend_thread(tid);
  if (t__analytics[ref].postcall)
    return ((int32 (*)(int32*, uint32))t__analytics[ref].postcall)(&result, tid);
  return result;
}

int32 __wrap_join_thread(uint32 tid) {
  int32 result, ref = JOIN_THREAD_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(int32*, uint32))t__analytics[ref].precall)(&result, tid))
    return result;

  int32 __real_join_thread(uint32);
  result = __real_join_thread(tid);
  if (t__analytics[ref].postcall)
    return ((int32 (*)(int32*, uint32))t__analytics[ref].postcall)(&result, tid);
  return result;
}

void __wrap_enqueue_thread(queue_t* qid, uint32 tid, int32 key) {
  int32 ref = THREAD_ENQUEUE_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(queue_t*, uint32, int32))t__analytics[ref].precall)(qid, tid, key))
    return;

  void __real_enqueue_thread(queue_t*, uint32, int32);
  __real_enqueue_thread(qid, tid, key);
  if (t__analytics[ref].postcall)
    ((void (*)(queue_t*, uint32, int32))t__analytics[ref].postcall)(qid, tid, key);
}

uint32 __wrap_dequeue_thread(queue_t* qid) {
  int32 result, ref = THREAD_DEQUEUE_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(int32*, queue_t*))t__analytics[ref].precall)(&result, qid))
    return result;

  int32 __real_dequeue_thread(queue_t*);
  result = __real_dequeue_thread(qid);
  if (t__analytics[ref].postcall)
    return ((uint32 (*)(int32*, queue_t*))t__analytics[ref].postcall)(&result, qid);
  return result;
}

int32 __wrap_sleep_thread(uint32 tid) {
  int32 result, ref = SLEEP_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(int32*, uint32))t__analytics[ref].precall)(&result, tid))
    return result;

  int32 __real_sleep_thread(uint32);
  result = __real_sleep_thread(tid);
  if (t__analytics[ref].postcall)
    return ((int32 (*)(int32*, uint32))t__analytics[ref].postcall)(&result, tid);
  return result;
}

int32 __wrap_unsleep_thread(uint32 tid) {
  int32 result, ref = UNSLEEP_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(int32*, uint32))t__analytics[ref].precall)(&result, tid))
    return result;

  int32 __real_unsleep_thread(uint32);
  result = __real_unsleep_thread(tid);
  if (t__analytics[ref].postcall)
    return ((int32 (*)(int32*, uint32))t__analytics[ref].postcall)(&result, tid);
  return result;
}

semaphore_t __wrap_create_sem(int32 count) {
  semaphore_t result;
  int32 ref = SEM_CREATE_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(semaphore_t*, uint32))t__analytics[ref].precall)(&result, count))
    return result;

  semaphore_t __real_create_sem(int32 count);
  result = __real_create_sem(count);
  if (t__analytics[ref].postcall)
    return ((semaphore_t (*)(semaphore_t*, int32))t__analytics[ref].postcall)(&result, count);
  return result;
}

int32 __wrap_free_sem(semaphore_t* sem) {
  int32 result, ref = SEM_FREE_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(int32*, semaphore_t*))t__analytics[ref].precall)(&result, sem))
    return result;

  int32 __real_free_sem(semaphore_t*);
  result = __real_free_sem(sem);
  if (t__analytics[ref].postcall)
    return ((int32 (*)(int32*, semaphore_t*))t__analytics[ref].postcall)(&result, sem);
  return result;
}

int32 __wrap_wait_sem(semaphore_t* sem) {
  int32 result, ref = SEM_WAIT_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(int32*, semaphore_t*))t__analytics[ref].precall)(&result, sem))
    return result;

  int32 __real_wait_sem(semaphore_t*);
  result = __real_wait_sem(sem);
  if (t__analytics[ref].postcall)
    return ((int32 (*)(int32*, semaphore_t*))t__analytics[ref].postcall)(&result, sem);
  return result;
}

int32 __wrap_post_sem(semaphore_t* sem) {
  int32 result, ref = SEM_POST_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(int32*, semaphore_t*))t__analytics[ref].precall)(&result, sem))
    return result;

  int32 __real_post_sem(semaphore_t*);
  result = __real_post_sem(sem);
  if (t__analytics[ref].postcall)
    return ((int32 (*)(int32*, semaphore_t*))t__analytics[ref].postcall)(&result, sem);
  return result;
}

void __wrap_init_heap(void) {
  int32 ref = HEAP_INIT_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)())t__analytics[ref].precall)())
    return;

  void __real_init_heap(void);
  __real_init_heap();
  if (t__analytics[ref].postcall)
    ((void (*)())t__analytics[ref].postcall)();
}

void* __wrap_malloc(uint32 len) {
  char* result;
  byte ref = MALLOC_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(void*, uint32))t__analytics[ref].precall)(&result, len))
    return (void*)result;

  void* __real_malloc(uint32);
  result = __real_malloc(len);
  if (t__analytics[ref].postcall)
    return ((void* (*)(void*, uint32))t__analytics[ref].postcall)(&result, len);
  return result;
}

void __wrap_free(void* ptr) {
  byte ref = FREE_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(void*))t__analytics[ref].precall)(ptr))
    return;

  void __real_free(void*);
  __real_free(ptr);
  if (t__analytics[ref].postcall)
    ((void (*)(void*))t__analytics[ref].postcall)(ptr);
}

void __wrap_init_tty(void) {
  int32 ref = TTY_INIT_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)())t__analytics[ref].precall)())
    return;

  void __real_init_tty(void);
  __real_init_tty();
  if (t__analytics[ref].postcall)
    ((void (*)())t__analytics[ref].postcall)();
}

char __wrap_tty_getc(void) {
  char result;
  int32 ref = TTY_GETC_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(char*))t__analytics[ref].precall)(&result))
    return (char)result;

  char __real_tty_getc(void);
  result = __real_tty_getc();
  if (t__analytics[ref].postcall)
    return ((char (*)(char*))t__analytics[ref].postcall)(&result);
  return result;
}

void __wrap_tty_putc(char c) {
  byte ref = TTY_PUTC_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(char))t__analytics[ref].precall)(c))
    return;

  void __real_tty_putc(char);
  __real_tty_putc(c);
  if (t__analytics[ref].postcall)
    ((void (*)(char))t__analytics[ref].postcall)(c);
}

int32 __wrap_create(char* fname) {
  int32 result, ref = FS_CREATE_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(int32*, char*))t__analytics[ref].precall)(&result, fname))
    return result;

  int32 __real_create(char*);
  result = __real_create(fname);
  if (t__analytics[ref].postcall)
    return ((int32 (*)(int32*, char*))t__analytics[ref].postcall)(&result, fname);
  return result;
}

int32 __wrap_open(char* fname) {
  int32 result, ref = FS_OPEN_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(int32*, char*))t__analytics[ref].precall)(&result, fname))
    return result;

  int32 __real_open(char*);
  result = __real_open(fname);
  if (t__analytics[ref].postcall)
    return ((int32 (*)(int32*, char*))t__analytics[ref].postcall)(&result, fname);
  return result;
}

int32 __wrap_close(int32 fd) {
  int32 result, ref = FS_CLOSE_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(int32*, int32))t__analytics[ref].precall)(&result, fd))
    return result;

  int32 __real_close(int32);
  result = __real_close(fd);
  if (t__analytics[ref].postcall)
    return ((int32 (*)(int32*, int32))t__analytics[ref].postcall)(&result, fd);
  return result;
}

void __wrap_setmaskbit_fs(uint32 block) {
  byte ref = FS_SETMASKBIT_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(uint32))t__analytics[ref].precall)(block))
    return;

  void __real_setmaskbit_fs(uint32);
  __real_setmaskbit_fs(block);
  if (t__analytics[ref].postcall)
    ((void (*)(uint32))t__analytics[ref].postcall)(block);
}

void __wrap_clearmaskbit_fs(uint32 block) {
  byte ref = FS_CLEARMASKBIT_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(uint32))t__analytics[ref].precall)(block))
    return;

  void __real_clearmaskbit_fs(uint32);
  __real_clearmaskbit_fs(block);
  if (t__analytics[ref].postcall)
    ((void (*)(uint32))t__analytics[ref].postcall)(block);
}

uint32 __wrap_getmaskbit_fs(uint32 block) {
  uint32 result, ref = FS_GETMASKBIT_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(uint32*, uint32))t__analytics[ref].precall)(&result, block))
    return result;

  uint32 __real_getmaskbit_fs(uint32);
  result = __real_getmaskbit_fs(block);
  if (t__analytics[ref].postcall)
    return ((uint32 (*)(uint32*, uint32))t__analytics[ref].postcall)(&result, block);
  return result;
}

void __wrap_mkfs(void) {
  byte ref = FS_MKFS_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)())t__analytics[ref].precall)())
    return;

  void __real_mkfs(void);
  __real_mkfs();
  if (t__analytics[ref].postcall)
    ((void (*)())t__analytics[ref].postcall)();
}

uint32 __wrap_mount_fs(void) {
  uint32 result, ref = FS_MOUNT_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(uint32*))t__analytics[ref].precall)(&result))
    return result;

  uint32 __real_mount_fs(void);
  result = __real_mount_fs();
  if (t__analytics[ref].postcall)
    return ((uint32 (*)(uint32*))t__analytics[ref].postcall)(&result);
  return result;
}

uint32 __wrap_umount_fs(void) {
  uint32 result, ref = FS_UMOUNT_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(uint32*))t__analytics[ref].precall)(&result))
    return result;

  uint32 __real_umount_fs(void);
  result = __real_umount_fs();
  if (t__analytics[ref].postcall)
    return ((uint32 (*)(uint32*))t__analytics[ref].postcall)(&result);
  return result;
}

uint32 __wrap_mk_ramdisk(uint32 szblocks, uint32 nblocks) {
  uint32 result, ref = BS_MK_RAMDISK_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(uint32*, uint32, uint32))t__analytics[ref].precall)(&result, szblocks, nblocks))
    return result;

  uint32 __real_mk_ramdisk(uint32, uint32);
  result = __real_mk_ramdisk(szblocks, nblocks);
  if (t__analytics[ref].postcall)
    return ((uint32 (*)(uint32*, uint32, uint32))t__analytics[ref].postcall)(&result, szblocks, nblocks);
  return result;
}

uint32 __wrap_free_ramdisk(void) {
  uint32 result, ref = BS_FREE_RAMDISK_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(uint32*))t__analytics[ref].precall)(&result))
    return result;
  
  uint32 __real_free_ramdisk(void);
  result = __real_free_ramdisk();
  if (t__analytics[ref].postcall)
    return ((uint32 (*)(uint32*))t__analytics[ref].postcall)(&result);
  return result;
}

uint32 __wrap_read_bs(uint32 block, uint32 offset, void* buf, uint32 len) {
  uint32 result, ref = BS_READ_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(uint32*, uint32, uint32, void*, uint32))t__analytics[ref].precall)(&result, block, offset, buf, len))
    return result;
  
  uint32 __real_read_bs(uint32, uint32, void*, uint32);
  result = __real_read_bs(block, offset, buf, len);
  if (t__analytics[ref].postcall)
    return ((uint32 (*)(uint32*, uint32, uint32, void*, uint32))t__analytics[ref].postcall)(&result, block, offset, buf, len);
  return result;
}

uint32 __wrap_write_bs(uint32 block, uint32 offset, void* buf, uint32 len) {
  uint32 result, ref = BS_WRITE_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(uint32*, uint32, uint32, void*, uint32))t__analytics[ref].precall)(&result, block, offset, buf, len))
    return result;
  
  uint32 __real_write_bs(uint32, uint32, void*, uint32);
  result = __real_write_bs(block, offset, buf, len);
  if (t__analytics[ref].postcall)
    return ((uint32 (*)(uint32*, uint32, uint32, void*, uint32))t__analytics[ref].postcall)(&result, block, offset, buf, len);
  return result;
}

int32 __wrap_raise_syscall(uint32 sig) {
  int32 result, ref = RAISE_SYSCALL_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(int32*, uint32))t__analytics[ref].precall)(&result, sig))
    return result;
  
  int32 __real_raise_syscall(uint32);
  result = __real_raise_syscall(sig);
  if (t__analytics[ref].postcall)
    return ((int32 (*)(int32*, uint32))t__analytics[ref].postcall)(&result, sig);
  return result;
}

int32 __wrap_resched(void) {
  int32 result, ref = RESCHED_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(int32*))t__analytics[ref].precall)(&result))
    return result;
  
  int32 __real_resched(void);
  result = __real_resched();
  if (t__analytics[ref].postcall)
    return ((int32 (*)(int32*))t__analytics[ref].postcall)(&result);
  return result;
}

void __wrap_uart_handler(void) {
  byte ref = UART_HANDLER_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)())t__analytics[ref].precall)())
    return;
  
  void __real_uart_handler(void);
  __real_uart_handler();
  if (t__analytics[ref].postcall)
    ((void (*)())t__analytics[ref].postcall)();
}

void __wrap_set_uart_interrupt(byte enabled) {
  byte ref = SET_UART_INTERRUPT_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(byte))t__analytics[ref].precall)(enabled))
    return;
  
  void __real_set_uart_interrupt(byte);
  __real_set_uart_interrupt(enabled);
  if (t__analytics[ref].postcall)
    ((void (*)(byte))t__analytics[ref].postcall)(enabled);
}

uint32 __wrap_read(uint32 fd, char* buff, uint32 len) {
  uint32 result, ref = FS_READ_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(uint32*, uint32, char*, uint32))t__analytics[ref].precall)(&result, fd, buff, len))
    return result;
  
  uint32 __real_read(uint32, char*, uint32);
  result = __real_read(fd, buff, len);
  if (t__analytics[ref].postcall)
    return ((uint32 (*)(uint32*, uint32, char*, uint32))t__analytics[ref].postcall)(&result, fd, buff, len);
  return result;
}

uint32 __wrap_write(uint32 fd, char* buff, uint32 len) {
  uint32 result, ref = FS_WRITE_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(uint32*, uint32, char*, uint32))t__analytics[ref].precall)(&result, fd, buff, len))
    return result;
  
  uint32 __real_write(uint32, char*, uint32);
  result = __real_write(fd, buff, len);
  if (t__analytics[ref].postcall)
    return ((uint32 (*)(uint32*, uint32, char*, uint32))t__analytics[ref].postcall)(&result, fd, buff, len);
  return result;
}

uint32 __wrap_seek(uint32 fd, uint32 offset, uint32 relative) {
  uint32 result, ref = FS_SEEK_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(uint32*, uint32, uint32, uint32))t__analytics[ref].precall)(&result, fd, offset, relative))
    return result;
  
  uint32 __real_seek(uint32, uint32, uint32);
  result = __real_seek(fd, offset, relative);
  if (t__analytics[ref].postcall)
    return ((uint32 (*)(uint32*, uint32, uint32, uint32))t__analytics[ref].postcall)(&result, fd, offset, relative);
  return result;
}

char __wrap_uart_putc(char ch) {
  char result;
  byte ref = UART_PUTC_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(char*, char))t__analytics[ref].precall)(&result, ch))
    return result;
  
  uint32 __real_uart_putc(char);
  result = __real_uart_putc(ch);
  if (t__analytics[ref].postcall)
    return ((char (*)(char*, char))t__analytics[ref].postcall)(&result, ch);
  return result;
}

char __wrap_uart_getc(void) {
  char result;
  byte ref = UART_GETC_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(char*))t__analytics[ref].precall)(&result))
    return result;
  
  uint32 __real_uart_getc(void);
  result = __real_uart_getc();
  if (t__analytics[ref].postcall)
    return ((char (*)(char*))t__analytics[ref].postcall)(&result);
  return result;
}

void __wrap_lock_mutex(byte* mutex) {
  byte ref = MUTEX_LOCK_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(byte*))t__analytics[ref].precall)(mutex))
    return;
  
  void __real_lock_mutex(byte*);
  __real_lock_mutex(mutex);
  if (t__analytics[ref].postcall)
    ((void (*)(byte*))t__analytics[ref].postcall)(mutex);
}

void __wrap_release_mutex(byte* mutex) {
  byte ref = MUTEX_RELEASE_REF;
  t__analytics[ref].numcalls += 1;
  if (t__analytics[ref].precall &&
      ((uint32 (*)(byte*))t__analytics[ref].precall)(mutex))
    return;
  
  void __real_release_mutex(byte*);
  __real_release_mutex(mutex);
  if (t__analytics[ref].postcall)
    ((void (*)(byte*))t__analytics[ref].postcall)(mutex);
}
