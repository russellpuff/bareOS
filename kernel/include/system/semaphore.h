#ifndef H_SEM
#define H_SEM

#include <system/queue.h>

#define S_FREE 0  /*  Macros for indicating if a semaphore entry is  */
#define S_USED 1  /*  free or currently in use.                      */

/*  Each semaphore's state is stored in a 'semaphore_t' structure found in the sem_table  *
 *    (see system/semaphore.c)                                                            */
typedef struct _sem {
  byte state;           /*  The current state of the semaphore (S_FREE or S_USED)  */
  queue_t queue;        /*  Queue containing all thread awaiting this semaphore    */
} semaphore_t;

/*  Semaphore related prototypes */
semaphore_t create_sem(int32_t);
int32_t free_sem(semaphore_t*);
int32_t wait_sem(semaphore_t*);
int32_t post_sem(semaphore_t*);

void create_mutex(byte*);
void lock_mutex(byte*);
void release_mutex(byte*);

#endif
