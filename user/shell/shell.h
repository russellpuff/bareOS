#ifndef H_SHELL
#define H_SHELL

#include <lib/barelib.h>

typedef byte (*function_t)(char*);

typedef struct {
	const char* name;
	function_t func;
} command_t;

byte builtin_echo(char*);
byte builtin_hello(char*);
byte builtin_cat(char*);
byte builtin_shutdown(char*);
byte builtin_reboot(char*);
byte builtin_clear(char*);
byte builtin_ls(char*);
byte builtin_cd(char*);
byte builtin_mkdir(char*);
function_t get_command(const char* name);

extern command_t builtin_commands[];
extern directory_t cwd;

#endif
