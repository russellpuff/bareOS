#include <lib/io.h>
#include <lib/string.h>
#include "shell.h"

/*
 * 'builtin_hello' prints "Hello, <text>!\n" where <text> is the contents 
 * following "builtin_hello " in the argument and returns 0.  
 * If no text exists, print and error and return 1 instead.
 */
byte builtin_hello(char* arg) {
  if(!strcmp(arg, "hello") || !strcmp(arg, "hello ")) {
    printf("Error - bad argument\n");
    return 1;
  }
  arg += 6;
  printf("Hello, %s!\n", arg);
  return 0;
}
