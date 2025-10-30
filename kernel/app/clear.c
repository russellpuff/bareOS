#include <lib/bareio.h>
#include <app/shell.h>

byte builtin_clear(char* arg) {
    uart_write("\x1b[3J\x1b[H\x1b[2J");
    return 0;
}