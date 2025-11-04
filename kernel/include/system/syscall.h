#ifndef H_SYSCALL
#define H_SYSCALL

typedef enum { RESCHED, TICK } syscall_signum;

int32_t raise_syscall(uint64_t);          /*  Ask the operating system to run a low level system function  */
extern void pend_resched(uint64_t);

#endif
