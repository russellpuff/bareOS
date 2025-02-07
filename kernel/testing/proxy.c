#include <barelib.h>
#include "proxy.h"
#include "complete.h"

//-----------------------------------------------
#ifndef MILESTONE_3
int32 create_thread(void* addr, char* arg, uint32 len) { return 0; }
int32 kill_thread(uint32 tid) { return 0; }
int32 resume_thread(uint32 tid) { return 0; }
int32 suspend_thread(uint32 tid) { return 0; }
int32 join_thread(uint32 tid) { return 0; }

void resched(void) { return; }
void ctxsw(uint32** dst, uint32** src) { return; }

thread_t thread_table[NTHREADS];
uint32 current_thread = 0;

#endif
//-----------------------------------------------
#ifndef MILESTONE_4
void enqueue_thread(uint32 tid, uint32 key) { return; }
uint32 dequeue_thread(uint32 tid) { return 0; }
queue_t queue_table[NTHREADS];
queue_t ready_list;
#endif
//-----------------------------------------------
#ifndef MILESTONE_5
queue_t sleep_list;
int32 sleep_thread(uint32 tid, uint32 duration) { return 0; }
int32 unsleep_thread(uint32 tid) { return 0; }
#endif
//-----------------------------------------------
#ifndef MILESTONE_6
semaphore_t sem_table[NSEM];
semaphore_t create_sem(int32 count) { return (semaphore_t){0}; }
int32 free_sem(uint32 sid) { return 0; }
int32 wait_sem(uint32 sid) { return 0; }
int32 post_sem(uint32 sid) { return 0; }
void lock_mutex(byte* mutex) { return; }
void release_mutex(byte* mutex) { return; }
#endif
//-----------------------------------------------
#ifndef MILESTONE_7
void init_heap(void) { return; }
void* malloc(uint64 len) {return NULL; }
void free(void*) { return; }
alloc_t* freelist;
#endif
//-----------------------------------------------
#ifndef MILESTONE_8
ring_buffer_t tty_in;
ring_buffer_t tty_out;
char tty_getc(void) { return 0; }
void tty_putc(char c) { return; }
void init_tty(void) { return; }
#endif
//-----------------------------------------------
#ifndef MILESTONE_9
bdev_t bs_stats(void) { return (bdev_t){ 0 }; }
uint32 mk_ramdisk(uint32 bsize, uint32 bnum) { return 0; }
uint32 free_ramdisk(void) { return 0; }
uint32 read_bs(uint32 a, uint32 b, void* c, uint32 d) { return 0; }
uint32 write_bs(uint32 a, uint32 b, void* c, uint32 d) { return 0; }

void setmaskbit_fs(uint32 bit) { return; }
void clearmaskbit_fs(uint32 bit) { return; }
uint32 getmaskbit_fs(uint32 bit) { return 0; }

void mkfs(void) { return; }
uint32 mount_fs(void) { return 0; }
uint32 umount_fs(void) { return 0; }

int32 create(char* fname) { return 0; }
int32 open(char* fname) { return 0; }
int32 close(int32 fd) { return 0; }

filetable_t oft[NUM_FD];
fsystem_t* fsd = NULL;
#endif
//-----------------------------------------------
#ifndef MILESTONE_10
uint32 read(uint32 fd, char* buff, uint32 len) { return 0; }
uint32 write(uint32 fd, char* buff, uint32 len) { return 0; }
uint32 seek(uint32 fd, uint32 offset, uint32 relative) { return 0; }
#endif
