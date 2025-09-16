/* File contains definitions for all shell based filesystem utility commands. */
#include <lib/bareio.h>
#include <lib/barelib.h>
#include <mm/malloc.h>
#include <fs/fs.h>

/* 'redirect_file' handles writing the output of a program to a file *
 * when > is used in the shell.                                      */
byte redirect_file(char* arg, char* filename) {
    return 0;
}

/* builtin_cat takes a file name and attempts to print its contents. *
 * It just assumes the files is in the current directory right now.  */
byte builtin_cat(char* arg) {
    arg += 4; /* Jump past arg0. TODO: This is very unsafe if you just type "cat". */
    int16_t fd = open(arg);
    if(fd == -1) {
        kprintf("%s - File not found.\n", arg);
        return 1;
    }
    uint32_t size = get_filesize(fd);
    char* buffer = malloc(size);
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
    for(byte i = 0; i < fsd->root_dir.numentries; ++i) {
        kprintf("&x%s%0  ", fsd->root_dir.entry[i].name);
    }
    kprintf("\n");
    return 0;
}

byte builtin_cd(char* arg) {
    return 0;
}

byte builtin_mkdir(char* arg) {
    return 0;
}

byte buintin_rmdir(char* arg) {
    return 0;
}