#include <lib/io.h>
#include <lib/string.h>
#include <lib/ecall.h>
#include "shell.h"

#define PROMPT "bareOS$ "  /*  Prompt printed by the shell to the user  */
/* Arbitrary limits. */
#define LINE_SIZE 1024
#define MAX_ARG0_SIZE 128
#define DIGITS(n) ((n) < 10 ? 1 : ((n) < 100 ? 2 : 3)) /* Swift way of discerning the char size of retval. */

command_t builtin_commands[] = {
    { "hello", (function_t)builtin_hello },
    { "echo", (function_t)builtin_echo },
    //{ "cat", (function_t)builtin_cat },
    { "shutdown", (function_t)builtin_shutdown },
    //{ "reboot", (function_t)builtin_reboot },
    { "clear", (function_t)builtin_clear },
    //{ "ls", (function_t)builtin_ls },
    { NULL, NULL }
};

function_t get_command(const char* name) {
    for(uint32_t i = 0; builtin_commands[i].name != NULL; ++i) {
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
int main(void) {
    byte last_retval = 0;
    while (1) {
        printf("[%u] &x%s&0", last_retval, PROMPT);
        char line[LINE_SIZE];
        uint32_t len = gets(line, LINE_SIZE);

        /* Extract first argument (program name). */
        char arg0[MAX_ARG0_SIZE + 1]; /* factor in null character */
        uint16_t ctr = 0;
        for (; ctr < MAX_ARG0_SIZE; ++ctr) {
            if (line[ctr] == ' ' || line[ctr] == '\0') { break; }
            arg0[ctr] = line[ctr];
        }
        arg0[ctr] = '\0';

        /* Replace line placeholders with enviornment variables. */
        //byte chSz = DIGITS(last_retval);
        char prompt[LINE_SIZE + 64]; /* Arbitrary extra space. */
        /*
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
        uint64_t prompt_len = (uint64_t)(p_ptr - prompt + 1);
        */

        memcpy(prompt, line, len); /* temp until sprintf is implemented */
        function_t func = get_command(arg0);
        if(func) { last_retval = func(prompt); } 
        else { last_retval = ecall_spawn(arg0, prompt); }
    }
    return 0;
}