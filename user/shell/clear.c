#include <lib/io.h>
#include "shell.h"

byte builtin_clear(char* arg) {
	(void)arg;
	printf("%s", "\x1b[3J\x1b[H\x1b[2J");
	return 0;
}