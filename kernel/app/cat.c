#include <bareio.h>
#include <barelib.h>
#include <fs.h>

byte builtin_cat(char* arg) {
    int16 fd = open(arg);
    if(fd == -1) {
        kprintf("%s - File not found.", arg);
        return -1;
    }
    uint16 BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE];
    read(fd, buffer, BUFFER_SIZE);
    close(fd);
    kprintf(buffer);
    return 0;
}