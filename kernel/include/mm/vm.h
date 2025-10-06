#ifndef H_VM
#define H_VM

#include <lib/barelib.h>

extern bool MMU_ENABLED;

#define KDM_BASE 0xFFFFFFC000000000UL
#define PAGE_SIZE 0x1000UL
#define PAGE_SHIFT 12

#define ALIGN_UP_4K(addr) ((void*)((((uint64_t)(addr)) + 0xFFFUL) & ~0xFFFUL))
#define ALIGN_UP_2M(addr) ((void*)((((uint64_t)(addr)) + ((1ULL<<21) - 1)) & ~((1ULL<<21) - 1)))
#define ADDR_TO_PPN(a) ((uint64_t)(a) >> PAGE_SHIFT)
#define PA_TO_KVA(pa) ((void*)((uint64_t)(pa) + (MMU_ENABLED ? KDM_BASE : 0)))
//#define KVA_TO_PA(kva) ((uint64_t)((uint64_t)(kva) - KDM_BASE))
#define PPN_TO_PA(ppn) ((uint64_t)(ppn) << PAGE_SHIFT)
#define PPN_TO_KVA(ppn) PA_TO_KVA(PPN_TO_PA(ppn))

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
void free_pages(uint64_t);

extern uint64_t kernel_root_ppn;

#endif