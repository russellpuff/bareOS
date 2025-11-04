#ifndef H_VM
#define H_VM

#include <lib/barelib.h>

extern volatile byte MMU_ENABLED;

#define KVM_BASE 0xFFFFFFC000000000UL
#define PAGE_SIZE 0x1000UL
#define PAGE_SHIFT 12

#define ALIGN_UP_4K(addr) ((void*)((((uint64_t)(addr)) + 0xFFFUL) & ~0xFFFUL))
#define ALIGN_UP_2M(addr) ((void*)((((uint64_t)(addr)) + ((1ULL<<21) - 1)) & ~((1ULL<<21) - 1)))
#define PA_TO_PPN(pa) ((uint64_t)(pa) >> PAGE_SHIFT)
#define PA_TO_KVA(pa) ((void*)((uint64_t)(pa) + (MMU_ENABLED ? KVM_BASE : 0)))
#define KVA_TO_PA(kva) ((void*)((uint64_t)(kva) - (MMU_ENABLED ? KVM_BASE : 0)))
#define PPN_TO_PA(ppn) ((uint64_t)(ppn) << PAGE_SHIFT)
#define PPN_TO_KVA(ppn) (PA_TO_KVA(PPN_TO_PA(ppn)))
#define KVA_TO_PPN(kva) (PA_TO_PPN(KVA_TO_PA(kva)))

typedef enum { ALLOC_4K, ALLOC_2M, ALLOC_PROC, ALLOC_IDLE } prequest;

typedef struct {
    uint64_t v : 1;      /* Valid bit - Page is valid for use                                      */
    uint64_t r : 1;      /* Write bit - Page is writable                                           */
    uint64_t w : 1;      /* Read bit - Page is readable                                            */
    uint64_t x : 1;      /* Execute bit - Page can execute instructions                            */
    uint64_t u : 1;      /* User bit - Page is accessible in user mode                             */
    uint64_t g : 1;      /* Global bit - Page is accessible by all processes                       */
    uint64_t a : 1;      /* Accessed bit - Page has been rwx since last cleared                    */
    uint64_t d : 1;      /* Dirty bit - Page has been written to since last cleared                */
    uint64_t _rsv2 : 2;  /* Reserved                                                               */
    uint64_t ppn : 44;   /* Physical page number that this entry points to, or the next level page */
    uint64_t _rsv1 : 10; /* Reserved                                                               */
} pte_t;

_Static_assert(sizeof(pte_t) == 8, "pte_t must be 8 bytes");

void init_pages(void);
uint64_t alloc_page(uint32_t);
void free_pages(uint64_t);
void free_process_pages(uint32_t);
void* translate_user_address(uint64_t, uint64_t);
int32_t copy_to_user(uint64_t, uint64_t, const void*, uint64_t);
int32_t zero_user(uint64_t, uint64_t, uint64_t);

extern uint64_t kernel_root_ppn;
extern byte* s_trap_top;

#endif