#ifndef H_STRING
#define H_STRING

#include <lib/barelib.h>

int16_t strcmp(const char*, const char*);
void* memset(void*, byte, uint64_t);
void* memcpy(void*, const void*, uint64_t);
int16_t memcmp(const void*, const void*, uint64_t);

#endif