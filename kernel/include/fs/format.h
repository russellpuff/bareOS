#ifndef H_FORMAT
#define H_FORMAT

#include <barelib.h>
#include <dev/io_iface.h>

/* Contains kernel-only format definitions. See io_iface.h for shared format definitions. */

#define FS_MAGIC 0x62726673 /* 'brfs' - bare ram filesystem */
#define FS_VERSION 2
#define BDEV_BLOCK_SIZE 512     /* Size of each block in bytes                      */
#define BDEV_NUM_BLOCKS 4096    /* Number of blocks in the block device             */
#define SB_BIT 0   /* Alias for the super block index                               */
#define BM_BIT 1   /* Alias for the bitmask block index                             */
#define FT_BIT 2   /* Alias for the start of the fat table block index              */
#define FAT_ITEM_SIZE (sizeof(int16_t)) /* Size of an entry in the FAT table in bytes. */
#define FT_LEN ((BDEV_NUM_BLOCKS * FAT_ITEM_SIZE) / BDEV_BLOCK_SIZE) /* Length of the fat table in blocks */
#define IN_BIT (FT_BIT + FT_LEN) /* Alias for the index of the default first intable block index. */

#define MAX_INTABLE_BLOCKS 128  /* Completely arbitrary. TODO: Replace array.       */
#define OFT_MAX 32              /* Also arbitrary.                                  */
#define IN_ERR 0 /* Returned when a new inode cannot be created. */

typedef enum { CLOSED, OPEN } FSTATE;
typedef enum { RD_ONLY, WR_ONLY, RDWR, APPEND } FMODE;

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
	uint16_t curr_index; /* Current offset within the file                                      */
	inode_t* inode;       /* Pointer to an in-memory copy of the inode, real one in FILE        */
	uint16_t in_index;   /* Index of file's inode                                               */
	bool in_dirty;       /* A flag saying whether the inode copy has to be written back         */
} filetable_t;

/* 'fsuper_t' represents serializable data stored in the super block, it contains key   *
 * data required to remake the fs after a remount. Many of its values are necessary for *
 * the in-RAM filesystem_t to function.                                                 */
typedef struct {
	uint32_t magic;        /* Magic number used to identify the type of filesystem      */
	uint8_t version;       /* The filesystem's version                                  */
	uint16_t fat_head;     /* Index of the first FAT table block                        */
	uint16_t fat_size;     /* The number of blocks the FAT table occupies               */
	uint16_t intable_head; /* Index of the first inode table block                      */
	uint16_t intable_size; /* Current capacity of innode table given allocated blocks   */
	dirent_t root_dirent;  /* Index in the inode table of the root directory inode      */
	uint16_t intable_blocks[MAX_INTABLE_BLOCKS]; /* FAT path of inode table blocks for O(1) lookup time */
	uint8_t intable_numblks; /* Number of blocks ACTUALLY used by the inode table       */
} fsuper_t;

/* 'fsystem_t' is the combined overarching master record of all the information about a *
 * ramdisk fs. It contains a pointer to the block device, a cached super, and an open   *
 * file table                                                                           */
typedef struct {
	bdev_t* device;
	fsuper_t super;
	filetable_t oft[OFT_MAX];
} fsystem_t;

/* 'drive_t' is the master authority of a block device. If unmounted, it frees fsd resource while keeping *
 * the bdev intact. If the drive is mounted again later, the fsd is remade from the bdev's super block.   */
typedef struct {
	uint8_t id;
	bdev_t bdev;
	fsystem_t* fsd;
	bool mounted;
} drive_t;

/* 'drv_reg' is a simple linked list of registered drives, once discovered and registered, * 
 * a drive becomes available for mount                                                     */
typedef struct drv_reg {
	drive_t drive;
	struct drv_reg* next;
} drv_reg;

/* 'mount_t' is a simple linked list of actively mounted drives that exposes their fsd and mount point (mp) */
typedef struct mount_t {
	fsystem_t* fsd;
	char* mp;
	struct mount_t* next;
} mount_t;

#endif
