#ifndef H_SYSCALL
#define H_SYSCALL

#include <lib/barelib.h>
#include <lib/ecall.h>

typedef enum { RESCHED, TICK } syscall_signum;

int32_t raise_syscall(uint64_t);          /*  Ask the operating system to run a low level system function  */
extern void pend_resched(uint64_t);
uint32_t handle_device_ecall(ecall_number, uint32_t, byte*);

#endif
