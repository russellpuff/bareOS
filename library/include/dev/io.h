#ifndef H_IO
#define H_IO

#include <barelib.h>
#include <dev/printf_iface.h>
#include <dev/io_iface.h>

void printf(const char*, ...);
void sprintf(byte*, const char*, ...);
int32_t gets(char*, uint32_t);

int8_t fcreate(const char*);
int8_t fopen(const char*, FILE*);
int8_t fclose(FILE*);
uint32_t fread(FILE*, byte*, uint32_t);
uint32_t fwrite(FILE*, byte*, uint32_t);
int8_t fdelete(const char*);
int8_t mkdir(const char*);
int8_t rmdir(const char*);
int8_t rddir(const char*, dirent_t*, uint32_t);
int8_t getdir(const char*, directory_t*, bool);

#endif
