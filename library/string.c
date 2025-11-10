#include <barelib.h>
#include <string.h>

/* Compares two strings and returns the difference between them (zero if equal) */
int16_t strcmp(const char* str1, const char* str2) {
  for(; *str1 == *str2; str1++, str2++) {
	if(*str1 == '\0') { return 0; }
  }
  return (unsigned char)*str1 - (unsigned char)*str2;
}

/* Returns the length of a string excluding its null terminator */
uint64_t strlen(const char* str) {
	const char* s = str;
	while (*s) ++s;
	return (uint64_t)(s - str);
}

/* Returns a pointer to the last instance of 'c' in a string, or NULL if not found */
char* strrchr(const char* s, char c) {
	const char* p = NULL;
	while (1) {
		if (*s == c)
			p = s;
		if (*s++ == '\0')
			return (char*)p;
	}
}

/* Returns a pointer to the first instance of a substring s2 found in s1 or null if not found */
char* strstr(const char* s1, const char* s2) {
	uint32_t n = strlen(s2);
	while (*s1)
		if (!memcmp(s1++, s2, n))
			return (char*)(s1 - 1);
	return NULL;
}

void* memset(void* s, uint8_t c, uint64_t n) {
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
