#include <bareio.h>
#include <barelib.h>
#include <string.h>

/*
 * 'builtin_echo' reads in a line of text from the UART
 * and prints it.  It will continue to do this until the
 * line read from the UART is empty (indicated with a \n
 * followed immediately by another \n).
 */
byte builtin_echo(char* arg) {
  const uint16 LINE_SIZE = 1024;
  uint16 char_cnt = 0;
  if(!strcmp(arg, "echo") || !strcmp(arg, "echo ")) {
    while(1) { 
      char line[LINE_SIZE]; 
      uint16 ctr = 0;
      do {
        line[ctr] = uart_getc();
        ++char_cnt;
      } while (line[ctr++] != '\n' && ctr < LINE_SIZE - 1);
      line[--ctr] = '\0';
      --char_cnt;
      if(!strcmp(line, "")) { return char_cnt; }
      kprintf("%s\n", line);
    }
  } else {
    arg += 5;
    kprintf("%s\n", arg);
  }
  return 0;
}
