#include <io.h>
#include <string.h>
#include "shell.h"

/*
 * 'builtin_echo' reads in a line of text from the UART
 * and prints it.  It will continue to do this until the
 * line read from the UART is empty (indicated with a \n
 * followed immediately by another \n).
 */
uint8_t builtin_echo(char* arg) {
  const uint32_t LINE_SIZE = 1024;
  uint32_t char_cnt = 0;
  if(strlen(arg) == 0) { 
	while(1) { 
	  char line[LINE_SIZE]; 
	  char_cnt += gets(line, LINE_SIZE);
	  if(strlen(line) == 0) { return (uint8_t)char_cnt; }
	  printf("%s\n", line);
	}
  } else {
	printf("%s\n", arg);
  }
  return (uint8_t)char_cnt;
}
