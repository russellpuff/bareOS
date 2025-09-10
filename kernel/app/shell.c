#include <lib/bareio.h>
#include <lib/barelib.h>
#include <lib/string.h>
#include <system/thread.h>
#include <app/shell.h>

#define PROMPT "bareOS$ "  /*  Prompt printed by the shell to the user  */
/* Arbitrary limits. */
#define LINE_SIZE 1024
#define MAX_ARG0_SIZE 128
#define DIGITS(n) ((n) < 10 ? 1 : ((n) < 100 ? 2 : 3)) /* Swift way of discerning the char size of retval. */

command_t builtin_commands[] = {
    { "hello", (function_t)builtin_hello },
    { "echo", (function_t)builtin_echo },
    { "cat", (function_t)builtin_cat },
    { "shutdown", (function_t)builtin_shutdown },
    { "clear", (function_t)builtin_clear },
    { "ls", (function_t)builtin_ls },
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
        kprintf("&x%s&0", PROMPT);
        char line[LINE_SIZE];
        uint32_t chars_read = get_line(line, LINE_SIZE);

        /* Extract first argument (program name). */
        char arg0[MAX_ARG0_SIZE + 1]; /* factor in null character */
        uint16_t ctr = 0;
        for (; ctr < MAX_ARG0_SIZE; ++ctr) {
            if (line[ctr] == ' ' || line[ctr] == '\0') { break; }
            arg0[ctr] = line[ctr];
        }
        arg0[ctr] = '\0';

        /* Replace line placeholders with enviornment variables. */
        byte chSz = DIGITS(last_retval);
        char prompt[LINE_SIZE + 64]; /* Arbitrary extra space. */
        char* l_ptr = line;
        char* p_ptr = prompt;
        while (*l_ptr != '\0') {
            if (*l_ptr == '$' && *(l_ptr + 1) == '?') {
                ksprintf((byte*)p_ptr, "%u", last_retval);
                p_ptr += chSz;
                l_ptr += 2;
            }
            else *p_ptr++ = *l_ptr++;
        }
        *p_ptr = '\0';

        /* Try to run a built-in program. In the future, try calling a user-installed program too. */
        function_t func = get_command(arg0);
        if(func) {
            uint32_t fid = create_thread(func, prompt, chars_read);
            resume_thread(fid);
            last_retval = join_thread(fid);
        } else {
            kprintf("%s: command not found\n", arg0);
        }
    }
    return 0;
}