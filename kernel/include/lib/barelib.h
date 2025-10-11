#ifndef H_BARELIB
#define H_BARELIB

/* Intellisense override for VS22 to suppress errors relating to bad clang support. */
#ifdef IS_OVERRIDE 
#define bool  _Bool
#define true  1
#define false 0
#endif

/*
 *  This header contains typedefs for common utility types and functions
 *  used by the kernel.
 */

//#define m_interrupt __attribute__ ((interrupt ("machine"))) void
//#define s_interrupt __attribute__ ((interrupt ("supervisor"))) void

#define NULL 0x0
#define va_copy(dst, src)       __builtin_va_copy(dst, src)    /*                                      */
#define va_start(last, va)      __builtin_va_start(last, va)   /*  GCC compiler builtins for variable  */
#define va_arg(va, type)        __builtin_va_arg(va, type)     /*  length arguments                    */
#define va_end(va)              __builtin_va_end(va)           /*                                      */

typedef unsigned char     byte;         /*                                            */
typedef char              int8_t;       /*                                            */
typedef unsigned char     uint8_t;      /*                                            */
typedef int               int32_t;      /*  Defines some clearer aliases for common   */
typedef short             int16_t;      /*  data types to make bit-width unambiguous  */
typedef unsigned int      uint32_t;     /*                                            */
typedef unsigned short    uint16_t;     /*                                            */
typedef long int          int64_t;      /*                                            */
typedef unsigned long int uint64_t;     /*  Always 8 bytes since RISCV64              */
typedef __builtin_va_list va_list;      /*                                            */

void init_uart(void);

extern byte text_start;    /*  Shares the address of the first instruction in the text segment                  */
extern byte text_end;      /*  Shares the address directly after the last instruction in the text segment       */
extern byte data_start;    /*  Shares the address of the first initialized global/static variable               */
extern byte data_end;      /*  Shares the address directly after the last initialized global/static variable    */
extern byte bss_start;     /*  Shares the address of the first uninitialized global/static variable             */
extern byte bss_end;       /*  Shares the address directly after the last uninitialized global/static variable  */
extern byte mem_start;     /*  Shares the address of the first heap/stack value                                 */
extern byte mem_end;       /*  Shares the address directly after the last possible heap/stack value             */

#endif
