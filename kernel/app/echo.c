#include <bareio.h>
#include <barelib.h>

int16 strcmp(const char* str1, const char* str2) {
  for(; *str1 == *str2; str1++, str2++) {
    if(*str1 == '\0') { return 0; }
  }
  return *(const unsigned char*)str1 - *(const unsigned char*)str2;
}

/*
 * 'builtin_echo' reads in a line of text from the UART
 * and prints it.  It will continue to do this until the
 * line read from the UART is empty (indicated with a \n
 * followed immediately by another \n).
 */
byte builtin_echo(char* arg) {
  const LINE_SIZE = 1024;
  if(!strcmp(arg, "echo") || !strcmp(arg, "echo ")) {
    while(1) { 
      char line[LINE_SIZE]; 
      uint16 ctr = 0;
      do {
      line[ctr] = uart_getc();
      } while (line[ctr++] != '\n' && ctr < LINE_SIZE - 1);
    line[ctr == LINE_SIZE ? ctr : --ctr] = '\0';
      if(!strcmp(line, "")) { return 0; }
      kprintf("\n%s\n", line);
    }
  } else {
    arg += 5;
    kprintf("%s", arg);
  }
  return 0;
}
