#ifndef H_THREAD
#define H_THREAD

#include <system/semaphore.h>
#include <fs/fs.h>
#include <barelib.h>

typedef enum {
	MODE_S,
	MODE_U
} thread_mode;

#define NTHREADS 20    /*  Maximum number of running threads  */

typedef enum {
	/*  These macros are not intended for  direct use.  Instead they  */
	/*  represent features a thread  may have and are combined below  */
	/*  to represent full states a thread  may be in.  They may also  */
	/*  be used as predicate filters to find threads with properties.  */
	THM_RUNNABLE = 0x1,
	THM_QUEUED = 0x2,
	THM_PAUSED = 0x4,
	THM_DEAD = 0x8,
	THM_TIMED = 0x10,

	/* Available states a thread can be in */
	TH_FREE = 0,
	TH_RUNNING = THM_RUNNABLE,
	TH_READY = THM_RUNNABLE | THM_QUEUED | THM_PAUSED,
	TH_SUSPEND = THM_PAUSED,
	TH_DEFUNCT = THM_DEAD,
	TH_SLEEP = THM_QUEUED | THM_PAUSED | THM_TIMED,
	TH_WAITING = THM_QUEUED | THM_PAUSED,
	TH_ZOMBIE = THM_RUNNABLE | THM_DEAD
} thread_state;

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
	uint64_t satp;      /* Fixed satp value of this thread                                         */
	thread_state state; /* The current state of the thread                                         */
	uint8_t retval;     /* The return value of the function (only valid when state == TH_DEFUNCT)  */
	semaphore_t sem;    /* Semaphore for the current thread                                        */
	byte* kstack_base;  /* Kernel VA, bottom of stack                                              */
	byte* kstack_top;   /* Kernel VA, top of stack                                                 */
	trapframe* tf;      /* Pointer to trapframe living in kstack                                   */
	context* ctx;       /* Pointer to context living in kstack                                     */
	thread_mode mode;   /* Determines whether a thread is running in supervisor or user mode       */
	dirent_t cwd;       /* Holds the process current working directory                             */
	uint32_t heap_sz;   /* Current size of process heap                                            */
} thread_t;

extern thread_t thread_table[];
extern uint32_t current_thread;    /*  The currently running thread  */
extern queue_t sleep_list;
extern semaphore_t reaper_sem; /* Global reaper sem zombie threads can post to */

/*  Thread related prototypes  */
void init_threads(void);
int32_t create_thread(void*, thread_mode);
int32_t join_thread(uint32_t);
int32_t kill_thread(uint32_t);
int32_t suspend_thread(uint32_t);
int32_t resume_thread(uint32_t);
int32_t sleep_thread(uint32_t, uint32_t);
int32_t unsleep_thread(uint32_t);
void user_thread_exit(trapframe* tf);

void resched(void*);
void reaper(void);

void context_switch(thread_t*, thread_t*);
void context_load(thread_t*, uint32_t);
extern void trapret(trapframe*);
extern void ctxsw(context*, context*, uint64_t, uint64_t);
extern void ctxload(uint64_t, uint64_t);

#endif
