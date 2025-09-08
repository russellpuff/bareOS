#ifndef H_SHELL
#define H_SHELL

typedef byte (*function_t)(char*);

typedef struct {
    const char* name;
    function_t func;
} command_t;

byte shell(char*);
byte builtin_echo(char*);
byte builtin_hello(char*);
byte builtin_cat(char*);
byte builtin_shutdown(char*);
function_t get_command(const char* name);

extern command_t builtin_commands[];

#endif
