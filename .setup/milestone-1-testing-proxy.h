#include <barelib.h>
#include "complete.h"

#ifndef TEST_PROXY_H
#define TEST_PROXY_H

#ifndef MILESTONE_3
#define TH_FREE  0
#define NTHREADS 0

typedef struct _thread {
  byte state;
  uint64* stackptr;
  uint32 parent;
  byte retval;
  uint32 priority;
} thread_t;

extern thread_t thread_table[];
extern uint32 current_thread;
extern byte mem_start;
extern byte mem_end;
#else
#include <thread.h>
#endif
// ------------------------------------------------
#ifndef MILESTONE_4
typedef struct {
  uint32 key;
  uint32 qprev;
  uint32 qnext;
} queue_t;

extern queue_t queue_table[];
extern queue_t ready_list;
#else
#include <queue.h>
#endif
// ------------------------------------------------
#ifndef MILESTONE_5
extern queue_t sleep_list;
#else
#include <sleep.h>
#endif
// ------------------------------------------------
#ifndef MILESTONE_6
#define NSEM 0
typedef struct {
  byte state;
  uint32 qprev;
  uint32 qnext;
} semaphore_t;

extern semaphore_t sem_table[];
#else
#include <semaphore.h>
#endif
// ------------------------------------------------
#ifndef MILESTONE_7
typedef struct _alloc {
  uint64 size;
  char state;
  struct _alloc* next;
} alloc_t;

void init_heap(void);
#else
#include <malloc.h>
#endif
// ------------------------------------------------
#ifndef MILESTONE_8
#define TTY_BUFFLEN 0
typedef struct {
  semaphore_t sem;
  char buffer[TTY_BUFFLEN];
  uint32 head;
  uint32 count;
} ring_buffer_t;

extern ring_buffer_t tty_in;
extern ring_buffer_t tty_out;
void init_tty(void);
#else
#include <tty.h>
#endif
// ------------------------------------------------
#ifndef MILESTONE_9
#define NUM_FD       0
#define DIR_SIZE     0
#define FILENAME_LEN 0
#define INODE_BLOCKS 0

typedef struct {
  uint32 id;
  uint32 size;
  uint32 blocks[INODE_BLOCKS];
} inode_t;

typedef struct {
  uint32 inode_block;
  char name[FILENAME_LEN];
} dirent_t;

typedef struct {
  uint32 numentries;
  dirent_t entry[DIR_SIZE];
} directory_t;

typedef struct {
  uint32 nblocks;
  uint32 blocksz;
} bdev_t;

typedef struct {
  bdev_t device;
  uint32 freemasksz;
  byte* freemask;
  directory_t root_dir;
} fsystem_t;

typedef struct {
  byte state;
  uint32 head;
  uint32 direntry;
  inode_t inode;
} filetable_t;

extern fsystem_t* fsd;
extern filetable_t oft[NUM_FD];
#else
#include <fs.h>
#endif
#endif
