#ifndef H_BAREIO
#define H_BAREIO

/*
 *  This header includes function prototypes for IO procedures
 *  used by the UART and other print/scan related functions.
 */

char uart_putc(char);
char uart_getc(void);
void kprintf(const char*, ...);

#endif
