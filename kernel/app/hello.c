#include <bareio.h>
#include <barelib.h>

/* I have to put this somewhere... */
int strcmp(const char* str1, const char* str2) {
  for(; *str1 == *str2; str1++, str2++) {
    if(*str1 == '\0') { return 0; }
  }
  return *(const unsigned char*)str1 - *(const unsigned char*)str2;
}

/*
 * 'builtin_hello' prints "Hello, <text>!\n" where <text> is the contents 
 * following "builtin_hello " in the argument and returns 0.  
 * If no text exists, print and error and return 1 instead.
 */
byte builtin_hello(char* arg) {
  if(!strcmp(arg, "hello") || !strcmp(arg, "hello ")) {
    kprintf("Error - bad argument\n");
    return 1;
  }
  arg += 6;
  kprintf("Hello, %s!\n", arg);
  return 0;
}
