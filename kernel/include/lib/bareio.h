#ifndef H_BAREIO
#define H_BAREIO

#include <barelib.h>

/*
 *  This header includes function prototypes for IO procedures
 *  used by the UART and other print/scan related functions.
 */

char putc(char);
char getc(void);
void uart_write(const char*);
void kprintf(const char*, ...);
void ksprintf(byte*, const char*, ...);
void krprintf(const char*, ...);
void vkrprintf(const char*, va_list);
uint32_t get_line(char*, uint32_t);

extern bool RAW_GETLINE;

#endif
