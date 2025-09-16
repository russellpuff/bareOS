#include <lib/barelib.h>
#include <lib/string.h>

int16_t strcmp(const char* str1, const char* str2) {
  for(; *str1 == *str2; str1++, str2++) {
    if(*str1 == '\0') { return 0; }
  }
  return (unsigned char)*str1 - (unsigned char)*str2;
}

uint32_t strlen(const char* str) {
    char* p = str;
    while (*p) p++;
    return p - str;
}

void* memset(void* s, byte c, int32_t n) {
    byte* p = (byte*)s;
    for(; n; --n) *p++ = c;
    return s;
}

void* memcpy(void* dst, const void* src, uint64_t n) {
    byte* d = (byte*)dst;
    const byte* s = (byte*)src;
    while (n--) *d++ = *s++;
    return dst;
}