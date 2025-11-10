#include <dev/io.h>
#include <dev/time.h>
#include <dev/ecall.h>
#include <util/string.h>
#include "shell.h"

/* File defines most basic builtin commands that are mostly unrelated to each other 
   "large" functions or groups of related functions should get their own file       */

/* 'builtin_help' prints the encoded command_t information for every builtin 
   Without the width formatting support of real printf, we gotta do this the hard way */
uint8_t builtin_help(char* arg) {
	(void)arg;
	printf("Available built-in commands:\n");

	uint32_t name_width = 0;
	uint32_t arg_width = 0;
	for (uint32_t i = 0; builtin_commands[i].name != NULL; ++i) {
		const char* args = builtin_commands[i].args ? builtin_commands[i].args : "";
		uint32_t name_len = (uint32_t)strlen(builtin_commands[i].name);
		uint32_t arg_len = (uint32_t)strlen(args);
		if (name_len > name_width) { name_width = name_len; }
		if (arg_len > arg_width) { arg_width = arg_len; }
	}

	for (uint32_t i = 0; builtin_commands[i].name != NULL; ++i) {
		const char* name = builtin_commands[i].name;
		const char* args = builtin_commands[i].args ? builtin_commands[i].args : "";
		const char* summary = builtin_commands[i].summary ? builtin_commands[i].summary : "";
		uint32_t name_len = (uint32_t)strlen(name);
		uint32_t arg_len = (uint32_t)strlen(args);
		printf("  %s", name);
		for (uint32_t pad = name_len; pad < name_width; ++pad) {
			printf(" ");
		}
		printf("  %s", args);
		for (uint32_t pad = arg_len; pad < arg_width; ++pad) {
			printf(" ");
		}
		if (*summary != '\0') {
			printf(" - %s", summary);
		}
		printf("\n");
	}
	return 0;
}


   /*
	* 'builtin_echo' reads in a line of text from the UART
	* and prints it.  It will continue to do this until the
	* line read from the UART is empty (indicated with a \n
	* followed immediately by another \n).
	*/
uint8_t builtin_echo(char* arg) {
	const uint32_t LINE_SIZE = 1024;
	uint32_t char_cnt = 0;
	if (strlen(arg) == 0) {
		while (1) {
			char line[LINE_SIZE];
			char_cnt += gets(line, LINE_SIZE);
			if (strlen(line) == 0) { return (uint8_t)char_cnt; }
			printf("%s\n", line);
		}
	}
	else {
		printf("%s\n", arg);
	}
	return (uint8_t)char_cnt;
}

/* Only supported arguments are 'now' and 'tz'
   If 'tz', you must pass in a string for new tz
   Gotta do this the hard way                    */
/* Todo: deshittify this builtin */
uint8_t builtin_time(char* arg) {
	if (strlen(arg) < 1) {
		printf("Error - needs an argument.\n");
		return 1;
	}
	if (arg[0] == 'n' && arg[1] == 'o' && arg[2] == 'w') {
		uint64_t time = rtc_read();
		if (arg[3] != '\0' && arg[4] == '-' && arg[5] == 's') {
			printf("%lu\n", time);
		}
		else {
			char str[TIME_BUFF_SZ];
			dt_to_string(seconds_to_dt(time), str, TIME_BUFF_SZ);
			printf("%s\n", str);
		}
		return 0;
	}
	if (arg[0] == 't' && arg[1] == 'z') {
		char* p = arg + 3;
		uint8_t code = rtc_chtz(p);
		const char* s = code == 0 ? "Successfully changed local timezone.\n" : "Failed to change local timezone.\n";
		printf("%s", s);
		return code;
	}
	return 1;
}

/* 'builtin_shutdown' and 'builtin_reboot' both ecall to write a magic     *
 * number to QEMU's syscon linked memory address which prompts an emulator *
 * shutdown or reboot respectively. They currently execute immediately     *
 * and kill the whole system without warning processes.                    */

uint8_t builtin_shutdown(char* arg) {
	ecall_pwoff();
	return 0; /* Unreachable */
}

/* This function doesn't work right now and is not registered as a shell command. */
uint8_t builtin_reboot(char* arg) {
	ecall_rboot();
	return 0; /* Unreachable */
}

uint8_t builtin_clear(char* arg) {
	(void)arg;
	printf("%s", "\x1b[3J\x1b[H\x1b[2J");
	return 0;
}