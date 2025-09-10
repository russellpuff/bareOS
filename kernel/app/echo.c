#include <lib/bareio.h>
#include <lib/barelib.h>
#include <lib/string.h>
#include <app/shell.h>

/*
 * 'builtin_echo' reads in a line of text from the UART
 * and prints it.  It will continue to do this until the
 * line read from the UART is empty (indicated with a \n
 * followed immediately by another \n).
 */
byte builtin_echo(char* arg) {
  const uint16_t LINE_SIZE = 1024;
  uint16_t char_cnt = 0;
  if(!strcmp(arg, "echo") || !strcmp(arg, "echo ")) {
    while(1) { 
      char line[LINE_SIZE]; 
      char_cnt += get_line(line, LINE_SIZE);
      if(!strcmp(line, "")) { return char_cnt; }
      kprintf("%s\n", line);
    }
  } else {
    arg += 5;
    kprintf("%s\n", arg);
  }
  return 0;
}
