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
	const uint32_t CHILD_MAX = 64, BUFF_SZ = 4096;
	dirent_t children[CHILD_MAX];
	uint8_t c = rddir(arg, children, CHILD_MAX);
	char b[BUFF_SZ];
	memset(b, '\0', BUFF_SZ);
	for (uint8_t i = 0; i < c; ++i) {
		char token = children[i].type == EN_FILE ? 'x' : 'b';
		sprintf((byte*)b, "&%c%s&0  ", token, children[i].name);
	}
	printf("%s", b);
	return 0;
}

byte builtin_cd(char* arg) {
	getdir(arg, &cwd, true);
	return 0;
}

byte builtin_mkdir(char* arg) {
	return 0;
}
