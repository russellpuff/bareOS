#ifndef H_BARELIB
#define H_BARELIB

/* Shared basic typedefs and utility macros available to both the kernel and user space.*/

/* Wrapper to trick intellisense into thinking newer C/gnu features are real
   For morons who use Visual Studio to develop this junk                     */
#ifdef __INTELLISENSE__
typedef enum { false, true } bool;
#define nullptr 0x0
#endif

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

#define offsetof(T, m) ((uint64_t)&(((T*)0)->m))

#endif
