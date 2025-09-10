/* File contains definitions for all shell based filesystem utility commands. */
#include <lib/bareio.h>
#include <lib/barelib.h>
#include <mm/malloc.h>
#include <fs/fs.h>

/* builtin_cat takes a file name and attempts to print its contents. *
 * It just assumes the files is in the current directory right now.  */
byte builtin_cat(char* arg) {
    if(arg[0] == 'c' && arg[1] == 'a' && arg[2] == 't' && arg[3] == ' ') arg += 4; /* Hotfix, deal with later. */
    int16_t fd = open(arg);
    if(fd == -1) {
        kprintf("%s - File not found.\n", arg);
        return 1;
    }
    uint32_t size = get_filesize(fd);
    char* buffer = malloc(++size);
    read(fd, buffer, size);
    close(fd);
    free(buffer);
    buffer[size] = '\0';
    kprintf("%s", buffer);
    return 0;
}

/* builtin_ls will print the contents of the current directory, until *
 * the ramdisk is modified to include more than one, it just iterates *
 * through the ramdisk root dir.                                      */
byte builtin_ls(char* arg) {
    /* Probably not a good idea to operate straight on the fs, TODO: fs wrappers and helper functions. */
    /* TODO: handle listing subdirectories. */
    for(int i = 0; i < fsd->root_dir.numentries; ++i) {
        kprintf("&x%s%0  ", fsd->root_dir.entry[i].name);
    }
    kprintf("\n");
    return 0;
}