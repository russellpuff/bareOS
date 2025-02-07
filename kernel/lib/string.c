#include <barelib.h>
#include <string.h>

int16 strcmp(const char* str1, const char* str2) {
  for(; *str1 == *str2; str1++, str2++) {
    if(*str1 == '\0') { return 0; }
  }
  return (unsigned char)*str1 - (unsigned char)*str2;
}