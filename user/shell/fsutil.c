/* File contains definitions for all shell based filesystem utility commands. */
#include <lib/io.h>
#include <lib/string.h>
#include "shell.h"

/* builtin_cat takes a file name and attempts to print its contents. *
 * It just assumes the files is in the current directory right now.  */
uint8_t builtin_cat(char* arg) {
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

uint8_t builtin_ls(char* arg) {
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

uint8_t builtin_cd(char* arg) {
	directory_t old = cwd;
	uint8_t status = getdir(arg, &cwd, true);
	if(status != 0) {
		printf("Error - failed to change directory\n");
		cwd = old;
		return status;
	}
	return 0;
}

uint8_t builtin_mkdir(char* arg) {
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
		printf("Error - %s\n", reason);
		return code * -1; /* this sucks lol */
	}
	return 0;
}

/* This test function is meant to replicate behavior observed in 
   bash when doing printf "some string" > file.txt 
   It requires the > character and looks for the rightmost one, so you
   can have > in the target string

   usage:
   'print this string gets printed > out.txt'
   out.txt is created and its contents are "this string gets printed\0"

   Will overwrite an existing file in its entirety. 
*/
uint8_t builtin_print(char* arg) {
	if (!arg || *arg == '\0') {
		printf("Error - path required\n");
		return 1;
	}
	char* sep = strrchr(arg, '>');
	if (!sep) {
		printf("Error - No separator\n");
		return 1;
	}

	if (sep == arg) { /* nothing before '>' */
		printf("Error - Nothing to write\n");
		return 1;        
	}
	if (*(sep - 1) != ' ' && sep[1] != ' ') { /* must be space before '>' */
		printf("Error - Bad format\n");
		return 1; 
	}

	/* terminate the text at the space before '>' */
	*(sep - 1) = '\0';

	/* path starts after ' > ' */
	char* path = sep + 2;
	if (*path == '\0') {
		printf("Error - No output path\n");
		return 1;
	}

	char* text = arg;
	uint32_t to_write = (uint32_t)strlen(text) + 1; /* include NUL */

	FILE f;
	if (fopen(path, &f) != 0) { 
		/* File doesn't exist, create it */
		if (fcreate(path) != 0) {
			printf("Error - Couldn't create file\n");
			return 1;
		}
		if (fopen(path, &f) != 0) {
			printf("Error - Couldn't open file\n");
			return 1;
		}
	}

	/* overwrite semantics */
	if (to_write >= f.inode.size) {
		fwrite(&f, (byte*)text, to_write);
	}
	else {
		/* pad with NULs up to existing size; arg length <= 1024 by shell limit */
		char buffer[1024];
		memcpy(buffer, text, to_write);
		memset(buffer + to_write, 0, f.inode.size - to_write);
		fwrite(&f, (byte*)buffer, f.inode.size);
	}

	fclose(&f);
	return 0;
}

uint8_t builtin_rm(char* arg) {
	if (arg == NULL || *arg == '\0') {
		printf("Error - path required\n");
		return 1;
	}

	int8_t status = fdelete(arg);
	if (status != 0) {
		int32_t code = status;
		if (code > 127) code -= 256;
		const char* reason = "unknown error";
		switch (code) {
			case -1: reason = "invalid path"; break;
			case -2: reason = "invalid file name"; break;
			case -3: reason = "file not found"; break;
			case -4: reason = "target is a directory"; break;
			case -5: reason = "file is open"; break;
			case -6: reason = "filesystem error"; break;
		default: break;
		}
		printf("Error - %s\n", reason);
		return code * -1; /* lousy */
	}
	return 0;
}

uint8_t builtin_rmdir(char* arg) {
	if (arg == NULL || *arg == '\0') {
		printf("Error - path required\n");
		return 1;
	}

	int8_t status = rmdir(arg);
	if (status != 0) {
		int32_t code = status;
		if (code > 127) code -= 256;
		const char* reason = "unknown error";
		switch (code) {
		case -1: reason = "invalid path"; break;
		case -2: reason = "invalid directory name"; break;
		case -3: reason = "cannot remove root"; break;
		case -4: reason = "directory not found"; break;
		case -5: reason = "target is not a directory"; break;
		case -6: reason = "directory not empty"; break;
		case -7: reason = "filesystem error"; break;
		default: break;
		}
		printf("Error - %s\n", reason);
		return code * -1;
	}
	return 0;
}
