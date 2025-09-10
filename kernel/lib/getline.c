#include <lib/barelib.h>
#include <lib/bareio.h>
#include <device/tty.h>

/* Gets a line from the UART / TTY and stores it into a buffer, converts the line into a C string. */
/* Returns the number of characters read. */
uint16_t get_line(char* buffer, uint16_t size) {
    if(size == 0) return 0;
    char* ptr = buffer;
    char* end = buffer + size - 1;
    while(1) {
        char ch = uart_getc();
        switch(ch) {
            case '\r':
            case '\n':
                *ptr = '\0';
                return (uint16_t)(ptr - buffer);
            case '\b':
            case 0x7f:
                if (ptr > buffer) { tty_bkspc(); --ptr; }
                break;
            default: /* TODO: handle nonprint characters I don't want. */
                if (ptr < end) *ptr++ = ch;
                break;
        }
    }
}
