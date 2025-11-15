#ifndef H_MALLOC
#define H_MALLOC

#include <barelib.h>

#define M_FREE 0  /*  Macros for indicating if a block of  */
#define M_USED 1  /*  memory is free or used               */
#define HEAP_START_VA 0x40000000UL /* Virtual start address of all virtual heaps */

/*  'alloc_t' structs contain the necessary state for tracking *
*   blocks of memory allocated to processes or free.           */
typedef struct _alloc {    /*                                               */
  uint64_t size;             /*  The size of the following block of memory    */
  char state;              /*  If the following block is free or allocated  */
  struct _alloc* next;     /*  A pointer to the next block of memory        */
} alloc_t;                 /*                                               */


/*  memory managmeent prototypes */
void init_heap(void);    /*  Create the initial space for processes to allocate memory  */
void mmu_regen_heap(void); /* Regenerate the heap once the MMU is available */
void* kmalloc(uint32_t);    /*  Allocate a block of memory for a process                   */
void free(void*);        /*  Return a block of memory to the free pool of memory        */

extern uint32_t k_heap_sz;

#endif
