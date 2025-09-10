#include <bareio.h>
#include <barelib.h>
#include <fs.h>

byte builtin_cat(char* arg) {
    if(arg[0] == 'c' && arg[1] == 'a' && arg[2] == 't' && arg[3] == ' ') arg += 4; /* Hotfix, deal with later. */
    int16_t fd = open(arg);
    if(fd == -1) {
        kprintf("%s - File not found.\n", arg);
        return 1;
    }
    const uint16_t BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE];
    read(fd, buffer, BUFFER_SIZE);
    close(fd);
    kprintf("%s\n", buffer);
    return 0;
}
