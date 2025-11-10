#include <string.h>
#include <io.h>
#include "shell.h"

/* Only supported arguments are 'now' and 'tz' 
   If 'tz', you must pass in a string for new tz 
   Gotta do this the hard way                    */
uint8_t builtin_time(char* arg) {
	if (strlen(arg) < 1) return 1;
	if (arg[0] == 'n' && arg[1] == 'o' && arg[2] == 'w') {
		uint64_t time = rtc_read();
		printf("%ul\n", time);
		return 0;
	}
	if (arg[0] == 't' && arg[1] == 'z') {
		char* p = arg + 3;
		if (strlen(p) < 3) return 1;
		uint8_t code = rtc_chtz(p);
		const char* s = code == 0 ? "Successfully changed local timezone.\n" : "Failed to change local timezone.\n";
		printf("%s", s);
		return 0;
	}
	return 1;
}
