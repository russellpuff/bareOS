#include <bareio.h>
#include <barelib.h>

int16 strcmp2(const char* str1, const char* str2) {
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
  const uint16 LINE_SIZE = 1024;
  uint16 char_cnt = 0;
  if(!strcmp2(arg, "echo") || !strcmp2(arg, "echo ")) {
    while(1) { 
      char line[LINE_SIZE]; 
      uint16 ctr = 0;
      do {
        line[ctr] = uart_getc();
        ++char_cnt;
      } while (line[ctr++] != '\n' && ctr < LINE_SIZE - 1);
      line[--ctr] = '\0';
      --char_cnt;
      if(!strcmp2(line, "")) { return char_cnt; }
      kprintf("%s\n", line);
    }
  } else {
    arg += 5;
    kprintf("%s\n", arg);
  }
  return 0;
}
