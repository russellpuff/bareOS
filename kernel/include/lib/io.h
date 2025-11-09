#ifndef H_IO
#define H_IO
#include <lib/barelib.h>

/* This file includes a few things copied from format.h without exposing all the fs details *
 * In the future, both the kernel layer and user layer will draw from a unified source      */

#define FILENAME_LEN 56 
#define MAX_PATH_DEPTH 32
#define MAX_PATH_LEN ((FILENAME_LEN * MAX_PATH_DEPTH) + MAX_PATH_DEPTH + 1)
typedef uint8_t FD;

typedef enum { EN_FREE, EN_BUSY, EN_FILE, EN_DIR } EN_TYPE;

typedef enum {
	FILE_CREATE,
	FILE_OPEN,
	FILE_READ,
	FILE_WRITE,
	FILE_TRUNCATE,
	DIR_CREATE,
	DIR_OPEN,
	DIR_READ,
	DIR_WRITE,
	DIR_TRUNCATE,
} DISKMODE;

//
// from format.h
//
typedef struct {
	uint32_t size;
	EN_TYPE type;
	int16_t head;
	int16_t parent;
	uint32_t modified;
} inode_t;

typedef struct {
	FD fd;
	inode_t inode;
} FILE;

typedef struct {
	uint16_t inode;
	EN_TYPE type;
	char name[FILENAME_LEN];
} dirent_t;

typedef struct {
	char path[MAX_PATH_LEN];
	dirent_t dir;
} directory_t;
//
//
//

/* Options for io ecall request */
typedef struct {
	byte* buffer;
	uint32_t length;
} uart_dev_opts;

/* Options for disk ecall request */
typedef struct {
	DISKMODE mode;
	FILE* file;
	byte* buff_in;
	byte* buff_out;
	uint32_t length;
} disk_dev_opts;

void printf(const char*, ...);
void sprintf(byte*, const char*, ...);
int32_t gets(char*, uint32_t);

int8_t fcreate(const char*);
int8_t fopen(const char*, FILE*);
int8_t fclose(FILE*);
uint32_t fread(FILE*, byte*, uint32_t);
uint32_t fwrite(FILE*, byte*, uint32_t);
int8_t fdelete(const char*);
int8_t mkdir(const char*);
int8_t rmdir(const char*);
int8_t rddir(const char*, dirent_t*, uint32_t);
int8_t getdir(const char*, directory_t*, bool);

#endif
