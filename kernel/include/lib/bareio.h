#ifndef H_BAREIO
#define H_BAREIO

#include <lib/barelib.h>

/*
 *  This header includes function prototypes for IO procedures
 *  used by the UART and other print/scan related functions.
 */

char uart_putc(char);
char uart_getc(void);
void uart_write(const char*);
void kprintf(const char*, ...);
void ksprintf(byte*, const char*, ...);
void krprintf(const char*, ...);
void vkrprintf(const char*, va_list);
uint16_t get_line(char*, uint16_t);

#endif
