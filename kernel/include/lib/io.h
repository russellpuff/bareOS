#ifndef H_IO
#define H_IO
#include <lib/barelib.h>

void printf(const char*, ...);
void sprintf(byte*, const char*, ...);
int32_t gets(char*, uint32_t);

#endif