#include <lib/io.h>
#include <lib/string.h>
#include <lib/ecall.h>
#include "shell.h"

#define PROMPT "bareOS"  /*  Prompt printed by the shell to the user  */
/* Arbitrary limits. */
#define LINE_SIZE 1024
#define MAX_ARG0_SIZE 128
#define DIGITS(n) ((n) < 10 ? 1 : ((n) < 100 ? 2 : 3)) /* Swift way of discerning the char size of retval. */

directory_t cwd;

command_t builtin_commands[] = {
	{ "echo", (function_t)builtin_echo },
	{ "cat", (function_t)builtin_cat },
	{ "shutdown", (function_t)builtin_shutdown },
	//{ "reboot", (function_t)builtin_reboot },
	{ "clear", (function_t)builtin_clear },
	{ "ls", (function_t)builtin_ls },
	{ "cd", (function_t)builtin_cd },
	{ "mkdir", (function_t)builtin_mkdir },
	{ "print", (function_t)builtin_print },
	{ "rm", (function_t)builtin_rm },
	{ "rmdir", (function_t)builtin_rmdir },
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
int32_t main(void) {
	uint8_t last_retval = 0;
	*cwd.path = '\0';
	builtin_cd("home");

	while (1) {
		printf("&x%s&0:&b%s&0$ ", PROMPT, cwd.path);
		char line[LINE_SIZE];
		gets(line, LINE_SIZE);

		/* Extract first argument (program name). */
		char arg0[MAX_ARG0_SIZE + 1]; /* factor in null character */
		uint16_t ctr = 0;
		for (; ctr < MAX_ARG0_SIZE; ++ctr) {
			if (line[ctr] == ' ' || line[ctr] == '\0') { break; }
			arg0[ctr] = line[ctr];
		}
		arg0[ctr] = '\0';
		if (strlen(arg0) == 0) continue;

		/* Replace line placeholders with enviornment variables. */
		uint8_t chSz = DIGITS(last_retval);
		char prompt[LINE_SIZE + 64]; /* Arbitrary extra space. */
		char* l_ptr = line + ctr;
		while (*l_ptr == ' ') ++l_ptr; /* Advance past whitespace and clean up */
		char* p_ptr = prompt;
		*p_ptr = '\0';
		while (*l_ptr != '\0') {
			if (*l_ptr == '$' && *(l_ptr + 1) == '?') {
				sprintf((byte*)p_ptr, "%u", last_retval);
				p_ptr += chSz;
				l_ptr += 2;
			}
			else *p_ptr++ = *l_ptr++;
		}
		*p_ptr = '\0'; 

		function_t func = get_command(arg0);
		if(func) { last_retval = func(prompt); } 
		else { last_retval = ecall_spawn(arg0, prompt); }
	}
	return 0;
}
