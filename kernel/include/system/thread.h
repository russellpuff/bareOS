#ifndef H_THREAD
#define H_THREAD

#include <lib/barelib.h>
#include <system/semaphore.h>

typedef enum {
	MODE_S,
	MODE_U
} thread_mode;

#define NTHREADS 20    /*  Maximum number of running threads  */

#define THM_RUNNABLE  0x1  /*  These macros are not intended for  direct use.  Instead they  */
#define THM_QUEUED    0x2  /*  represent features a thread  may have and are combined below  */
#define THM_PAUSED    0x4  /*  to represent full states a thread  may be in.  They may also  */
#define THM_DEAD      0x8  /*  be used as predicate filters to find threads with properties.  */
#define THM_TIMED    0x10

#define TH_FREE    0                                      /*                                                 */
#define TH_RUNNING THM_RUNNABLE                            /*  Threads can be in one of several states        */
#define TH_READY   (THM_RUNNABLE | THM_QUEUED | THM_PAUSED)  /*  This list will be extended as we add features  */
#define TH_SUSPEND THM_PAUSED                              /*                                                 */
#define TH_DEFUNCT THM_DEAD                                /*                                                 */
#define TH_SLEEP (THM_QUEUED | THM_PAUSED | THM_TIMED)       /*                                                 */
#define TH_WAITING (THM_QUEUED | THM_PAUSED)
#define TH_ZOMBIE (THM_RUNNABLE | THM_DEAD)

/* The context struct is used to hold information about kernel context during a context switch */
typedef struct {
	uint64_t ra;
	uint64_t s[12];
	uint64_t sp;
} context;

/* Trapframe is a more robust struct used to hold GSRs and privledged registers to assure full *
 * resume when exiting a trap. Both this and context are stored in a dedicated per-process     *
 * kernel stack region which is used to safely stash this info during traps/ctxsw to prevent   *
 * data corruption in more complicated use cases                                               */
typedef struct {
	uint64_t ra, sp, gp, tp;
	uint64_t t0, t1, t2, t3, t4, t5, t6;
	uint64_t s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11;
	uint64_t a0, a1, a2, a3, a4, a5, a6, a7;
	uint64_t sepc, sstatus;
} trapframe;

/*  Each thread has a corresponding 'thread_t' record in the 'thread_table' (see system/create.c)  */
/*  These entries contain information about the thread                                             */
typedef struct {
	uint64_t* stackptr; /* A pointer to the highest stack address for the thread                   */
	uint64_t root_ppn;  /* Physical page number of this thread's root page                         */
	uint32_t priority;  /* Thread priority (0=highest MAX_UINT32=lowest)                           */
	uint32_t parent;    /* The index into the 'thread_table' of the thread's parent                */
	uint16_t asid;      /* Address space identifier for this thread. For now, it's just the ID     */
	byte state;         /* The current state of the thread                                         */
	byte retval;        /* The return value of the function (only valid when state == TH_DEFUNCT)  */
	semaphore_t sem;    /* Semaphore for the current thread                                        */
	byte* kstack_base;  /* Kernel VA, bottom of stack                                              */
	byte* kstack_top;   /* Kernel VA, top of stack                                                 */
	trapframe* tf;      /* Pointer to trapframe living in kstack                                   */
	context* ctx;       /* Pointer to context living in kstack                                     */
	char* argptr;       /* Holds the arg to the process this thread runs with                      */
	thread_mode mode;   /* Determines whether a thread is running in supervisor or user mode       */
} thread_t;

extern thread_t thread_table[];
extern uint32_t current_thread;    /*  The currently running thread  */
extern queue_t sleep_list;

/*  Thread related prototypes  */
void init_threads(void);
int32_t create_thread(void*, char*, uint32_t, thread_mode);
int32_t join_thread(uint32_t);
int32_t kill_thread(uint32_t);
int32_t suspend_thread(uint32_t);
int32_t resume_thread(uint32_t);
int32_t sleep_thread(uint32_t, uint32_t);
int32_t unsleep_thread(uint32_t);
void user_thread_exit(trapframe* tf);

void resched(void);
void reaper(void);

void context_switch(thread_t*, thread_t*);
void context_load(thread_t*, uint32_t);
extern void trapret(trapframe*);
extern void ctxsw(context*, context*, uint64_t, uint64_t);
extern void ctxload(uint64_t, uint64_t);

#endif
