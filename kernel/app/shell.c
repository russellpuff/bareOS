#include <bareio.h>
#include <barelib.h>
#include <shell.h>
#include <string.h>

#define PROMPT "bareOS$ "  /*  Prompt printed by the shell to the user  */
#define LINE_SIZE 1024

/*
 * 'shell' loops forever, prompting the user for input, then calling a function based
 * on the text read in from the user.
 */
byte shell(char* arg) {
    byte last_retval = 0;
    while (1) {
        kprintf("%s", PROMPT);
        char line[LINE_SIZE]; /* Kinda want to extract this to a function bc it's used elsewhere but I need malloc. */
        get_line(line, LINE_SIZE);

        /* Extract first argument (program name). 10 char string is an arbitrary limit. */
        char arg0[11];
        int ctr = 0;
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
          kprintf("%s: command not found\n", arg0);
        }
    }
    return 0;
}
