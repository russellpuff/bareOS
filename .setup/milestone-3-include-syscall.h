#ifndef H_SYSCALL
#define H_SYSCALL

#define RESCHED 0                     /*  Index into the `syscall_table` containing the resched function (see `system/traps.c`) */

int32 raise_syscall(uint32);          /*  Ask the operating system to run a low level system function  */

#endif
