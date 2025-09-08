#include <barelib.h>
#include <bareio.h>

/* Gets a line from the UART / TTY and stores it into a buffer, converts the line into a C string. */
/* Returns the number of characters read. */
uint16 get_line(char* buffer, uint16 size) {
    uint16 count = 0;
    char* ptr = buffer;
    while(count < size - 1) {
        char ch = uart_getc();
        if(ch == '\n') break;
        if(ch == '\b' && count > 0) {
            uart_write("\b \b");
            --ptr; --count;
        } else {
            *ptr++ = ch;
            ++count;
        }
    }
    *ptr = '\0';
    return count;
}
