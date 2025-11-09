#ifndef H_SHELL
#define H_SHELL

#include <lib/barelib.h>

typedef uint8_t (*function_t)(char*);

typedef struct {
	const char* name;
	function_t func;
} command_t;

uint8_t builtin_echo(char*);
uint8_t builtin_print(char*);
uint8_t builtin_cat(char*);
uint8_t builtin_shutdown(char*);
uint8_t builtin_reboot(char*);
uint8_t builtin_clear(char*);
uint8_t builtin_ls(char*);
uint8_t builtin_cd(char*);
uint8_t builtin_mkdir(char*);
uint8_t builtin_rm(char*);
uint8_t builtin_rmdir(char*);
function_t get_command(const char* name);

extern command_t builtin_commands[];
extern directory_t cwd;

#endif
