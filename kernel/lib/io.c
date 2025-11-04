#include <lib/barelib.h>
#include <lib/printf.h>
#include <lib/io.h>
#include <lib/ecall.h>
#include <lib/string.h>

#define UPRINTF_BUFF_SIZE 1024

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
    io_dev_opts req;
    req.length = strlen(buffer);
    ecall_write(UART_DEV_NUM, (byte*)&req, (byte*)buffer);
}

void sprintf(byte* buffer, const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    printf_core(MODE_BUFFER, buffer, format, ap);
    va_end(ap);
}

int32_t gets(char* buffer, uint32_t length) {
    if (buffer == NULL || length == 0) return 0;
    io_dev_opts req;
    req.length = length;
    return (int32_t)ecall_read(UART_DEV_NUM, (byte*)&req, (byte*)buffer);
}