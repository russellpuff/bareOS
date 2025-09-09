#include <bareio.h>
#include <shell.h>

byte builtin_clear(char* arg) {
    uart_write("\x1b[2J\x1b[H");
    return 0;
}