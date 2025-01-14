#ifndef H_SLEEP
#define H_SLEEP

#include <barelib.h>
#include <thread.h>
#include <queue.h>

#define TH_SLEEP (TH_QUEUED | TH_PAUSED | TH_TIMED)

extern queue_t sleep_list;

/*  sleep related prototypes */
int32 sleep_thread(uint32, uint32);
int32 unsleep_thread(uint32);

#endif
