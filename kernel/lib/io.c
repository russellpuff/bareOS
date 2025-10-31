#include <lib/barelib.h>
#include <lib/printf.h>
#include <lib/io.h>
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

void sprintf(byte* buffer, const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    printf_core(MODE_BUFFER, buffer, format, ap);
    va_end(ap);
}

int32_t gets(char* buffer, uint32_t length) {
    if (buffer == NULL || length == 0) return 0;
    return (int32_t)ecall_read(UART_DEV_NUM, NULL, buffer, length);
}