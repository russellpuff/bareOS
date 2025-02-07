#include <bareio.h>
#include <barelib.h>
#include <shell.h>
#define PROMPT "bareOS$ "  /*  Prompt printed by the shell to the user  */
#define LINE_SIZE 1024

/* Doesn't bareOS kind of sound like "bare ass"? */
int16 strcmp(const char* str1, const char* str2) {
  for(; *str1 == *str2; str1++, str2++) {
    if(*str1 == '\0') { return 0; }
  }
  return (unsigned char)*str1 - (unsigned char)*str2;
}

/*
 * 'shell' loops forever, prompting the user for input, then calling a function based
 * on the text read in from the user.
 */
byte shell(char* arg) {
    byte last_retval = 0;
    while (1) {
        kprintf("%s", PROMPT);
        char line[LINE_SIZE]; /* Kinda want to extract this to a function bc it's used elsewhere but I need malloc. */
        uint16 ctr = 0;
        do {
            line[ctr] = uart_getc();
        } while (line[ctr++] != '\n' && ctr < LINE_SIZE - 1);
        line[--ctr] = '\0';

        /* Extract first argument (program name). 10 char string is an arbitrary limit. */
        char arg0[11];
        ctr = 0;
        for (; ctr < 10; ++ctr) {
            if (line[ctr] == ' ' || line[ctr] == '\0') { break; }
            arg0[ctr] = line[ctr];
        }
        arg0[ctr] = '\0';

        /* TODO: only run this code if you actually have to. */
        char digits[3];
        ctr = (last_retval < 10) ? 1 : (last_retval < 100) ? 2 : 3;
        if (last_retval == 0) { digits[0] = '0'; }
        else {
            for (uint16 q = ctr; q--; last_retval /= 10)
            {
                digits[q] = (last_retval % 10) + '0';
            }
        }

        uint16 count = 0;
        while (line[count] != '\0') {
            if (line[count] == '$' && line[count + 1] == '?') {
                switch (ctr) { /* TODO: refactor this to be less stupid */
                case 1:
                    line[count] = digits[0];
                    for (uint16 foo = ++count; line[foo] != '\0'; ++foo) {
                        line[foo] = line[foo + 1];
                    }
                    break;
                case 2:
                    line[count++] = digits[0];
                    line[count] = digits[1];
                    break;
                case 3:
                    line[count++] = digits[0];
                    line[count++] = digits[1];
                    char waiting = digits[2];
                    uint16 bar = count;
                    for (; line[bar] != '\0'; ++bar) {
                        char t = line[bar];
                        line[bar] = waiting;
                        waiting = t;
                    }
                    if (bar + 1 != LINE_SIZE) { /* If this causes the string to grow larger than line size, truncates the end on purpose. */
                        line[bar++] = waiting;   /* Will fix some other time, write shorter prompts shitlord. */
                    }
                    line[bar] = '\0';
                    break;
                }
            }
            else { ++count; }
        }

        if(!strcmp(arg0, "hello")) {
          last_retval = builtin_hello(line);
        } else if (!strcmp(arg0, "echo")) {
          last_retval = builtin_echo(line);
        } else {
          kprintf("Unknown command\n");
        }
    }
    return 0;
}
