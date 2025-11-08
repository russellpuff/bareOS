/* File contains definitions for all shell based filesystem utility commands. */
#include <lib/io.h>
#include <lib/string.h>
#include "shell.h"

/* builtin_cat takes a file name and attempts to print its contents. *
 * It just assumes the files is in the current directory right now.  */
byte builtin_cat(char* arg) {
	if (strlen(arg) <= 1) {
		printf("Nothing to cat\n");
		return 0;
	}

	FILE f;
	fopen(arg, &f);
	if (f.fd == (FD)-1) {
		printf("%s - File not found.\n", arg);
		return 1;
	}

	byte buffer[4096]; /* Placeholder until malloc */
	fread(&f, buffer, f.inode.size);
	fclose(&f);
	buffer[f.inode.size] = '\0';
	printf("%s", buffer);
	return 0;
}

byte builtin_ls(char* arg) {
	const uint32_t CHILD_MAX = 64;
	dirent_t children[CHILD_MAX];
	uint8_t c = rddir(arg, children, CHILD_MAX);

	for (uint8_t i = 0; i < c; ++i) {
		char token = children[i].type == EN_FILE ? 'x' : 'b';
		printf("&%c%s&0  ", token, children[i].name);
	}
	printf("\n");
	return 0;
}

byte builtin_cd(char* arg) {
	directory_t old = cwd;
	uint8_t status = getdir(arg, &cwd, true);
	if(status != 0) {
		printf("failed to change directory: %u\n", status);
		cwd = old;
		return status;
	}
	return 0;
}

byte builtin_mkdir(char* arg) {
	int8_t status = mkdir(arg);
	if (status != 0) {
		int32_t code = status;
		if (code > 127) code -= 256;
		const char* reason = "unknown error";
		switch (code) {
			case -1: reason = "invalid path"; break;
			case -2: reason = "invalid directory name"; break;
			case -3: reason = "already exists"; break;
			case -4: reason = "allocation failure"; break;
		default: break;
		}
		printf("mkdir failed (%d): %s\n", code, reason);
	}
	return 0;
}
