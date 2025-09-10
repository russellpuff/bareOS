#ifndef H_SLEEP
#define H_SLEEP

#include <lib/barelib.h>
#include <system/thread.h>
#include <system/queue.h>

#define TH_SLEEP (TH_QUEUED | TH_PAUSED | TH_TIMED)

extern queue_t sleep_list;

/*  sleep related prototypes */
int32_t sleep_thread(uint32_t, uint32_t);
int32_t unsleep_thread(uint32_t);

#endif
