#ifndef H_MEMLAYOUT
#define H_MEMLAYOUT

#include <barelib.h>

/*
 * Kernel-only linker symbols describing the in-memory layout of bareOS.
 */
extern byte text_start;
extern byte text_end;
extern byte data_start;
extern byte data_end;
extern byte bss_start;
extern byte bss_end;
extern byte mem_start;
extern byte mem_end;
extern byte ramdisk_start;
extern byte krsvd;

#endif
