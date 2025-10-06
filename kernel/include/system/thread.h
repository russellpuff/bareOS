#ifndef H_THREAD
#define H_THREAD

#include <lib/barelib.h>
#include <system/semaphore.h>

#define NTHREADS 20    /*  Maximum number of running threads  */

#define TH_RUNNABLE  0x1  /*  These macros are not intended for  direct use.  Instead they  */
#define TH_QUEUED    0x2  /*  represent features a thread  may have and are combined below  */
#define TH_PAUSED    0x4  /*  to represent full states a thread  may be in.  They may also  */
#define TH_DEAD      0x8  /*  be used as predicate filters to find threads with properties.  */
#define TH_TIMED    0x10

#define TH_FREE    0                                      /*                                                 */
#define TH_RUNNING TH_RUNNABLE                            /*  Threads can be in one of several states        */
#define TH_READY   (TH_RUNNABLE | TH_QUEUED | TH_PAUSED)  /*  This list will be extended as we add features  */
#define TH_SUSPEND TH_PAUSED                              /*                                                 */
#define TH_DEFUNCT TH_DEAD                                /*                                                 */
#define TH_SLEEP (TH_QUEUED | TH_PAUSED | TH_TIMED)       /*                                                 */
#define TH_WAITING (TH_QUEUED | TH_PAUSED)


/*  Each thread has a corresponding 'thread_t' record in the 'thread_table' (see system/create.c)  */
/*  These entries contain information about the thread                                             */
typedef struct _thread {
	uint64_t* stackptr; /* A pointer to the highest stack address for the thread                   */
	uint64_t root_ppn;  /* Physical page number of this thread's root page                         */
	uint32_t priority;  /* Thread priority (0=highest MAX_UINT32=lowest)                           */
	uint32_t parent;    /* The index into the 'thread_table' of the thread's parent                */
	uint16_t asid;      /* Address space identifier for this thread. For now, it's just the ID     */
	byte state;         /* The current state of the thread                                         */
	byte retval;        /* The return value of the function (only valid when state == TH_DEFUNCT)  */
	uint32_t _rsv1;     /* Reserved for furture use (padding)                                      */
	semaphore_t sem;    /* Semaphore for the current thread                                        */
} thread_t;

extern thread_t thread_table[];
extern uint32_t current_thread;    /*  The currently running thread  */
extern queue_t sleep_list;


/*  thread related prototypes  */
void init_threads(void);
int32_t create_thread(void* proc, char* arg, uint32_t arglen);
int32_t join_thread(uint32_t);
int32_t kill_thread(uint32_t);
int32_t suspend_thread(uint32_t);
int32_t resume_thread(uint32_t);
int32_t sleep_thread(uint32_t, uint32_t);
int32_t unsleep_thread(uint32_t);

void resched(void);

void ctxsw(thread_t*, thread_t*);
void ctxload(thread_t*);

#endif
