#ifndef H_IO_IFACE
#define H_IO_IFACE

/* The current value of FILENAME_LEN is chosen to pad out dirent_t to 64 bytes for simple iterating. */
#define FILENAME_LEN 56  /* Arbitrary maximum length of a filename in the FS */
#define MAX_PATH_DEPTH 32 /* Arbitrary path depth limit for searching */
#define MAX_PATH_LEN ((FILENAME_LEN * MAX_PATH_DEPTH) + MAX_PATH_DEPTH + 1) /* Enough for max depth count filenames plus '/' for each */
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

/* 'inode_t' are stored in the block device and contain all of the information needed by the file system      *
 * to read from and write to a given file or directory. Eact dirent (a file or directory) has a single inode. *
 * All inodes are serialized to the block device in a secret non-inode chain of blocks, walkable via the fsd  *
 * using fsd->device.intable_blocks, each element in this array is the same as walking a FAT path.            */
typedef struct {
	uint32_t size;       /* Size of the data in bytes                                       */
	EN_TYPE type;        /* Type of entry: file or directory | free means safe to overwrite */
	int16_t head;        /* Index to the head block                                         */
	int16_t parent;      /* Index of inode table entry holding this entry's parent          */
	uint32_t modified;   /* UNUSED - Last modified in Unix time                             */
} inode_t;

/* 'FILE' wraps the index into the oft with a copy of the inode
   A temporary solution until a better stat method comes about  */
typedef struct {
	FD fd;
	inode_t inode;
} FILE;

/* 'dirent_t' are directory entries (either a file or directory) that have a single inode associated with     *
 * them. Each group of dirent_t inside a directory have their struct data serialized to the parent's inode    *
 * blocks.                                                                                                    */
typedef struct {
	uint16_t inode;          /* Index of the inode table entry holding this entry's inode       */
	EN_TYPE type;            /* Type of entry: file or directory | free means safe to overwrite */
	char name[FILENAME_LEN]; /* The entry's human-readable name                                 */
} dirent_t;

/* 'directory_t' is a simple struct that pairs a string path with a dirent_t *
 * Its primary purpose is to let the shell keep track of both forms of cwd   */
typedef struct {
	char path[MAX_PATH_LEN];
	dirent_t dir;
} directory_t;

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

#endif
