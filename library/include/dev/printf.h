#ifndef H_PRINTF
#define H_PRINTF

#include <barelib.h>

#define MODE_REGULAR 0
#define MODE_BUFFER  1
#define MODE_RAW     3

/* Impl must provide this. Returns updated ptr. */
extern byte* printf_putc(char c, uint8_t mode, byte* ptr);

void printf_core(uint8_t mode, byte* ptr, const char* format, va_list ap);

#endif
