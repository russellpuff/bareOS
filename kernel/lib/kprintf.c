#include <lib/bareio.h>
#include <lib/barelib.h>
#include <lib/printf.h>
#include <mm/vm.h>

/* Helper functions. printf_putc is the only thing that can advance the buffer pointer so always return it for updates */
byte* printf_putc(char c, byte mode, byte* ptr) {
    switch(mode) {
        case MODE_REGULAR: uart_putc(c); break;
        case MODE_BUFFER: *ptr++ = c; break;
        case MODE_RAW: /* temporary implementation */
            #define UART0_CFG_REG 0x10000000
            #define UART0_RW_REG   0x0 
            #define UART0_STAT_REG 0x5
            #define UART_IDLE     0x20
            byte* uart = (byte*)PA_TO_KVA(UART0_CFG_REG);
            while ((uart[UART0_STAT_REG] & UART_IDLE) == 0); /*  Wait for the UART to be idle */
            uart[UART0_RW_REG] = (c & 0xff);                 /*  Send character to the UART   */
            break;
    }
    return ptr;
}

void kprintf(const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    printf_core(MODE_REGULAR, NULL, format, ap);
    va_end(ap);
}

void ksprintf(byte* buff, const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    printf_core(MODE_BUFFER, buff, format, ap);
    va_end(ap);
}

/* Kernel raw printf, bypasses tty to talk to the uart directly. Only for panic() and debug. */
void krprintf(const char* format, ...) {
    va_list ap; va_start(ap, format);
    vkrprintf(format, ap);
    va_end(ap);
}

/* Only panic() should use this */
void vkrprintf(const char* format, va_list ap) {
    printf_core(MODE_RAW, NULL, format, ap);
}

/* Takes a preformatted user string and dumps it into the tty. Only for use with print syscall. */
void kuprintf(const char* format, uint32_t len) {
    kprintf(format);
    //for (int i = 0; i < len; ++i) printf_putc(*format++, MODE_REGULAR, NULL);
}