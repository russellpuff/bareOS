#ifndef H_FORMAT
#define H_FORMAT

#include <lib/barelib.h>

#define FS_MAGIC 0x62726673 /* 'brfs' - bare ram filesystem */
#define FS_VERSION 2

#define FILENAME_LEN 32         /* Arbitrary maximum length of a filename in the FS */
#define BDEV_BLOCK_SIZE 512     /* Size of each block in bytes                      */
#define BDEV_NUM_BLOCKS 4096    /* Number of blocks in the block device             */
#define SB_BIT 0   /* Alias for the super block index                               */
#define BM_BIT 1   /* Alias for the bitmask block index                             */
#define FT_BIT 2   /* Alias for the start of the fat table block index              */
#define FT_LEN 16  /* Length of the fat table in blocks                             */
#define IN_BIT 3   /* Alias for the index of the default first intable block index. */

#define FAT_ITEM_SIZE sizeof(int16_t) /* Size of an entry in the FAT table in bytes. */

#define MAX_INTABLE_BLOCKS 128  /* Completely arbitrary. TODO: Replace array.       */
#define OFT_MAX 32              /* Also arbitrary.                                  */
#define IN_ERR 0 /* Returned when a new inode cannot be created. */

typedef enum { FREE, BUSY, FILE, DIR } EN_TYPE;
typedef enum { CLOSED, OPEN } FSTATE;
typedef enum { RD_ONLY, WR_ONLY, RDWR, APPEND } FMODE;

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

/* 'dirent_t' are directory entries (either a file or directory) that have a single inode associated with     *
 * them. Each group of dirent_t inside a directory have their struct data serialized to the parent's inode    *
 * blocks.                                                                                                    */ 
typedef struct {
	uint16_t inode;          /* Index of the inode table entry holding this entry's inode       */
	EN_TYPE type;            /* Type of entry: file or directory | free means safe to overwrite */
	char name[FILENAME_LEN]; /* The entry's human-readable name                                 */
} dirent_t;

/* The file system contains a 'bdev_t' that stores information about the underlying block device   */
typedef struct {
	uint16_t num_blocks;   /* The total number of blocks in the block device. May not equal the constant  */
	uint16_t block_size;   /* The size of each block in bytes. May not equal the constant                 */
	byte* ramdisk;         /* A pointer to the head of the ramdisk block region                           */
} bdev_t;

/* The 'filetable_t' struct is used to store information about a currently open file. An array  *
 * of these makes up the oft in the fsd that represents all open files. Each entry is           *
 * associated with a file when it is opened and deassociated with it is closed.                 */
typedef struct {
	FSTATE state;        /* The current state of the entry, either FSTATE OPEN or CLOSED        */
	FMODE mode;          /* The current ops mode. Read, write, read/write, or append            */
	uint16_t curr_block; /* The FAT index of the current block being looked at by ops           */
	uint16_t curr_index; /* Which MDEV_BLOCK_SIZE-sized chunk in the file curr_block represents */
	inode_t inode;       /* In-memory copy of the inode.                                        */
	bool in_dirty;       /* A flag saying whether the inode copy has to be written back         */
} filetable_t;

/* 'fsuper_t' represents serializable data stored in the super block, it contains key   *
 * data required to remake the fs after a reboot. Many of its values are necessary for  *
 * the in-RAM filesystem_t to function.                                                 */
typedef struct {
	uint32_t magic;        /* Magic number used to identify the type of filesystem      */
	byte version;          /* The filesystem's version                                  */
	uint16_t fat_head;     /* Index of the first FAT table block                        */
	uint16_t fat_size;     /* The number of blocks the FAT table occupies               */
	uint16_t intable_head; /* Index of the first inode table block                      */
	uint16_t intable_size; /* Current capacity of innode table given allocated blocks   */
	uint16_t root_inode;   /* Index in the inode table of the root directory inode      */
	uint16_t intable_blocks[MAX_INTABLE_BLOCKS]; /* FAT path of inode table blocks for O(1) lookup time */
	byte intable_numblks;  /* Number of blocks ACTUALLY used by the inode table         */
} fsuper_t;

/* 'fsystem_t' is the combined overarching master record of all the information about the *
 * ramdisk fs. In bareOS, there is only one 'fsystem_t' instance in memory called fsd.    */
typedef struct {
	bdev_t device;
	fsuper_t super;
	filetable_t oft[OFT_MAX];
} fsystem_t;

#endif
