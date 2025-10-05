#ifndef H_VM
#define H_VM

#include <lib/barelib.h>

#define PAGE_SIZE 0x1000UL
#define PAGE_SHIFT 12
#define ALIGN_UP_4K(addr) ((void*)((((uint64_t)(addr)) + 0xFFFUL) & ~0xFFFUL))
#define ALIGN_UP_2M(addr) ((void*)((((uint64_t)(addr)) + ((1ULL<<21) - 1)) & ~((1ULL<<21) - 1)))
#define ADDR_TO_PPN(a) ((uint64_t)(a) >> PAGE_SHIFT)
#define PPN_TO_ADDR(ppn) ((void*)((ppn) << PAGE_SHIFT))
#define STACK_SZ 

typedef enum { ALLOC_4K, ALLOC_2M, ALLOC_PROC, ALLOC_IDLE } prequest;

typedef struct {
	uint32_t _rsv1 : 10;    /* Reserved                                                               */
	uint64_t ppn : 44;      /* Physical page number that this entry points to, or the next level page */
	uint32_t _rsv2 : 2;     /* Reserved                                                               */
	byte d : 1;             /* Dirty bit - Page has been written to since last cleared                */
	byte a : 1;             /* Accessed bit - Page has been rwx since last cleared                    */
	byte g : 1;             /* Global bit - Page is accessible by all processes                       */
	byte u : 1;             /* User bit - Page is accessible in user mode                             */
	byte x : 1;             /* Execute bit - Page can execute instructions                            */
	byte r : 1;             /* Read bit - Page is readable                                            */
	byte w : 1;             /* Write bit - Page is writable                                           */
	byte v : 1;             /* Valid bit - Page is valid for use.                                     */
} pte_t;

void init_pages(void);
uint64_t alloc_page(prequest, uint64_t*, uint64_t*);

extern uint64_t kernel_root_ppn;

#endif