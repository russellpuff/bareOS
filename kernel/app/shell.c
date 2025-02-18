#include <bareio.h>
#include <barelib.h>
#include <shell.h>
#include <string.h>
#include <thread.h>

#define PROMPT "bareOS$ "  /*  Prompt printed by the shell to the user  */
#define LINE_SIZE 1024
#define MAX_ARG0_SIZE 10

command_t builtin_commands[] = {
    { "hello", (function_t)builtin_hello },
    { "echo", (function_t)builtin_echo },
    { NULL, NULL }
};

function_t get_command(const char* name) {
    for(int i = 0; builtin_commands[i].name != NULL; ++i) {
        if(!strcmp(name, builtin_commands[i].name)) {
            return builtin_commands[i].func;
        }
    }
    return NULL;
}

/*
 * 'shell' loops forever, prompting the user for input, then calling a function based
 * on the text read in from the user.
 */
byte shell(char* arg) {
    byte last_retval = 0;
    while (1) {
        kprintf("%s", PROMPT);
        char line[LINE_SIZE];
        uint32 chars_read = get_line(line, LINE_SIZE);

        /* Extract first argument (program name). */
        char arg0[MAX_ARG0_SIZE + 1]; /* factor in null character */
        uint16 ctr = 0;
        for (; ctr < MAX_ARG0_SIZE; ++ctr) {
            if (line[ctr] == ' ' || line[ctr] == '\0') { break; }
            arg0[ctr] = line[ctr];
        }
        arg0[ctr] = '\0';

        /* Replace line placeholders with enviornment variables. */
        bool rvRequest = false;
        uint16 count = 0;
        while (line[count] != '\0') {
            if (line[count] == '$' && line[count + 1] == '?') {
                char digits[3];
                if(!rvRequest) {
                    ctr = (last_retval < 10) ? 1 : (last_retval < 100) ? 2 : 3;
                    if (last_retval == 0) { digits[0] = '0'; }
                    else {
                        for (uint16 q = ctr; q--; last_retval /= 10)
                        {
                            digits[q] = (last_retval % 10) + '0';
                        }
                    }
                    rvRequest = true;
                }

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
                        line[bar++] = waiting;  /* Will fix some other time, write shorter prompts shitlord. */
                    }
                    line[bar] = '\0';
                    break;
                }
            }
            else { ++count; }
        }

        /* Try to run a built-in program. In the future, try calling a user-installed program too. */
        function_t func = get_command(arg0);
        if(func) {
            uint32 fid = create_thread(func, line, chars_read);
            resume_thread(fid);
            last_retval = join_thread(fid);
        } else {
            kprintf("%s: command not found\n", arg0);
        }
    }
    return 0;
}