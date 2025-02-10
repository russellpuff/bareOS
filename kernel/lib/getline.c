#include <barelib.h>
#include <bareio.h>

/* Gets a line from the UART but you must pass in the buffer. Returns number of characters read. */
uint16 get_line(char* buffer, uint16 size) {
    uint16 count = 0;
    for(; count < size - 1; ++buffer, ++count) {
        *buffer = uart_getc();
        if(*buffer == '\n') { break; }
    }
    *buffer = '\0';
    return count;
}
