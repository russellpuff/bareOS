#include <lib/barelib.h>
#include <lib/printf.h>
#include <lib/uprintf.h>
#include <lib/ecall.h>
#include <lib/string.h>

#define UPRINTF_BUFF_SIZE 1024
#define UART_DEV_NUM 0

byte* printf_putc(char c, byte mode, byte* ptr) {
    (void)mode;
    *ptr++ = c;
    return ptr;
}

void printf(const char* format, ...) {
    char buffer[UPRINTF_BUFF_SIZE];
    memset(buffer, '\0', UPRINTF_BUFF_SIZE);
    va_list ap;
    va_start(ap, format);
    printf_core(MODE_BUFFER, (byte*)buffer, format, ap);
    va_end(ap);
    uint64_t n = strlen(buffer);
    ecall_write(UART_DEV_NUM, NULL, buffer, n);
}