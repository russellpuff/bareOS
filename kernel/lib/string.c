#include <lib/barelib.h>
#include <lib/string.h>

int16_t strcmp(const char* str1, const char* str2) {
  for(; *str1 == *str2; str1++, str2++) {
    if(*str1 == '\0') { return 0; }
  }
  return (unsigned char)*str1 - (unsigned char)*str2;
}

void* memset(void* s, byte c, uint64_t n) {
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

int16_t memcmp(const void* s1, const void* s2, uint64_t n) {
    const byte* p1 = (const byte*)s1;
    const byte* p2 = (const byte*)s2;
    for (; n > 0; --n, ++p1, ++p2) {
        if (*p1 != *p2) {
            return (int16_t)*p1 - (int16_t)*p2;
        }
    }
    return 0;
}