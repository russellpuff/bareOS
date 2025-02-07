#include <barelib.h>
#include <bareio.h>

/* Gets a line from the UART but you must pass in the buffer due to lack of malloc. Returns number of characters read. */
uint16 get_line(char* buffer, uint16 size) {
    uint16 i = 0, count = 0;
    do {
        *buffer = uart_getc();
        ++count;
    } while (*buffer++ != '\n' && i < size - 1);
    *(--buffer) = '\0';
    return --count;
}