#include <util/string.h>
#include "shell.h"

/* File defines some stuff for builtin command searching and handling
   Definitions of the actual functions are in the other c files       */

/* command_t format: 
   name = exact command required to execute
   func = entry point to the command code
   args = arg helper for running the command
   summary = self-explanatory
 */
command_t builtin_commands[] = {
	{ "help", builtin_help, "(none)",
		"Display usage information for the built-in shell commands." },
	{ "echo", builtin_echo, "[text]",
		"Print the provided text or if none, read and echo lines until an empty line." },
	{ "cat", builtin_cat, "<path>",
		"Output the contents of the requested file." },
	{ "shutdown", builtin_shutdown, "(none)",
		"Power off the system immediately using the emulator control register." },
	//{ "reboot", builtin_reboot, "(none)",
	//	"Restart the system using the emulator control register." },
	{ "clear", builtin_clear, "(none)",
		"Clear the terminal display and reset the cursor position." },
	{ "ls", builtin_ls, "[path]",
		"List directory entries for the provided path or the current directory." },
	{ "cd", builtin_cd, "<path>",
		"Change the active working directory to the requested location." },
	{ "mkdir", builtin_mkdir, "<path>",
		"Create a directory at the specified path if it does not already exist." },
	{ "print", builtin_print, "<text> > <path>",
		"Write the provided text into the target file, overwriting existing data." },
	{ "rm", builtin_rm, "<path>",
		"Remove the file at the given path from the filesystem." },
	{ "rmdir", builtin_rmdir, "<path>",
		"Remove the directory at the given path if it is empty." },
	{ "time", builtin_time, "now [-s] | tz <tz>",
		"Read the RTC for the current time or update the system timezone." },
	{ NULL, NULL, NULL, NULL }
};

function_t get_command(const char* name) {
	for (uint32_t i = 0; builtin_commands[i].name != NULL; ++i) {
		if (!strcmp(name, builtin_commands[i].name)) {
			return builtin_commands[i].func;
		}
	}
	return NULL;
}