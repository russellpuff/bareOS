#ifndef TESTING_H
#define TESTING_H

#include <barelib.h>

#define SYS_RESET 250

/*
  Function Reference IDs
 */
#define INIT_REF                0
#define SUP_START_REF           1
#define DISP_KERN_REF           2
#define START_REF               3
#define SHELL_REF               4
#define HELLO_REF               5
#define ECHO_REF                6
#define CREATE_THREAD_REF       7
#define KILL_THREAD_REF         8
#define CTXSW_REF               9
#define RESUME_THREAD_REF      10
#define SUSPEND_THREAD_REF     11
#define JOIN_THREAD_REF        12
#define THREAD_ENQUEUE_REF     13
#define THREAD_DEQUEUE_REF     14
#define SLEEP_REF              15
#define UNSLEEP_REF            16
#define SEM_CREATE_REF         17
#define SEM_FREE_REF           18
#define SEM_WAIT_REF           19
#define SEM_POST_REF           20
#define HEAP_INIT_REF          21
#define MALLOC_REF             22
#define FREE_REF               23
#define TTY_INIT_REF           24
#define TTY_GETC_REF           25
#define TTY_PUTC_REF           26
#define FS_CREATE_REF          27
#define FS_OPEN_REF            28
#define FS_CLOSE_REF           29
#define FS_SETMASKBIT_REF      30
#define FS_CLEARMASKBIT_REF    31
#define FS_GETMASKBIT_REF      32
#define FS_MKFS_REF            33
#define FS_MOUNT_REF           34
#define FS_UMOUNT_REF          35
#define BS_MK_RAMDISK_REF      36
#define BS_FREE_RAMDISK_REF    37
#define BS_READ_REF            38
#define BS_WRITE_REF           39
#define DISABLE_INTERRUPTS_REF 40
#define RESTORE_INTERRUPTS_REF 41
#define RAISE_SYSCALL_REF      42
#define RESCHED_REF            43
#define UART_HANDLER_REF       44
#define SET_UART_INTERRUPT_REF 45
#define FS_READ_REF            46
#define FS_WRITE_REF           47
#define FS_SEEK_REF            48
#define UART_PUTC_REF          49
#define UART_GETC_REF          50
#define HANDLE_SYSCALL_REF     51
#define DELEG_CLK_REF          52
#define MUTEX_LOCK_REF         53
#define MUTEX_RELEASE_REF      54
#define T_MAX_REF 55

/*
  Utility definitions
 */
#define T_MAX_TESTS 30
#define T_MAX_ERR_LEN 128

typedef struct {
  uint32 numcalls;
  uint32 (*precall)();
  uint32 (*postcall)();
} analytic_t;

typedef struct {
  const char* category;
  uint32 ntests;
  const char* prompts[T_MAX_TESTS];
  char results[T_MAX_TESTS][T_MAX_ERR_LEN];
} tests_t;

extern analytic_t t__analytics[T_MAX_REF];

void t__initialize_tests(tests_t*, int);
void t__reset_analytics(void);
char* t__build_result(char*, int, const char*, ...);
void t__mask_uart(char*, uint32, char*, uint32);
void t__unmask_uart(void);
void t__print_results(void);
void t__write_timeout(void);
void t__reset(void);
void t__setup_testsuite(void);
byte assert(int32, char*, char*, ...);

#endif
