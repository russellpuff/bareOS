#include <lib/io.h>
#include <lib/string.h>
#include "shell.h"

/*
 * 'builtin_echo' reads in a line of text from the UART
 * and prints it.  It will continue to do this until the
 * line read from the UART is empty (indicated with a \n
 * followed immediately by another \n).
 */
byte builtin_echo(char* arg) {
  const uint32_t LINE_SIZE = 1024;
  uint32_t char_cnt = 0;
  if(!strcmp(arg, "echo") || !strcmp(arg, "echo ")) {
    while(1) { 
      char line[LINE_SIZE]; 
      char_cnt += gets(line, LINE_SIZE);
      if(!strcmp(line, "")) { return char_cnt; }
      printf("%s\n", line);
    }
  } else {
    arg += 5;
    printf("%s\n", arg);
  }
  return (byte)char_cnt;
}
