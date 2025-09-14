#ifndef H_FORMAT
#define H_FORMAT

#include <lib/barelib.h>

#define FS_MAGIC 0x62726673
#define FS_VERSION 2

#define FILENAME_LEN 32         /* Arbitrary maximum length of a filename in the FS           */
#define MDEV_BLOCK_SIZE 512     /* Size of each block in bytes                                */
#define MDEV_NUM_BLOCKS 4096    /* Number of blocks in the block device                       */

/* 'inode_t' are stored in the block device and contain all of the information needed by the             *
 * file system to read and write to/from a given file.  Each file has a single 'inode'.                  */
typedef struct inode {
	uint32_t id;                      /* Unique 'id' of the inode, corresponds with the block index          */
	uint32_t size;                    /* The size of the file in bytes                                       */
	uint32_t blocks[INODE_BLOCKS];    /* An array containing the indices of the blocks allocated to the file */
} inode_t;


/* The root directory contains DIR_SIZE directory entries.  This 'dirent_t' contains the metadata  *
 * necessary for mapping filenames to file inodes.                                                 */
typedef struct dirent {
	uint32_t inode_block;            /* The index of the block containing the inode for this file */
	char name[FILENAME_LEN];       /* The name associated with this file in a directory         */
} dirent_t;


/* The root directory is represented as a 'directory_t' structure in the file system.  It contains *
 * the list of entries in the direcotory.                                                          */
typedef struct {
	uint32_t numentries;             /* The number of entries used in the directory   */
	dirent_t entry[DIR_SIZE];      /* An array of 'dirent_t' containing the entries */
} directory_t;


/* The file system contains a 'bdev_t' that stores information about the underlying block device   */
typedef struct {
	uint32_t nblocks;                /* The total number of blocks in the block device */
	uint32_t blocksz;                /* The size of each block in bytes                */
} bdev_t;


/* The 'fsystem_t' is the master record that directly or indirectly contains all of the information *
 * about the file system.  In bareOS, there is one 'fsystem_t' instance called 'fsd'                *
 *    (see system/fs.c)                                                                             */
typedef struct {
	bdev_t device;                 /* The 'bdev_t' the describes the FS's block device                  */
	uint32_t freemasksz;             /* The size of the bitmask storing the free/used bits for each block */
	byte* freemask;                /* A pointer to the free bitmask, each bit corresponds to a block    */
	directory_t root_dir;          /* The 'directory_t' that stores the root directory information      */
} fsystem_t;

/* The 'filetable_t' struct is used to store information about a currently open file. It is used in   *
 * one place, an open file table ('oft') of 'filetable_t' that represent a list of open files.        *
 * Each entry is associated with a file when it is opened and removed with it is closed.              */
typedef struct {
	byte state;                    /* The current state of the entry, either FSTATE_OPEN or FSTATE_CLOSED */
	uint32_t head;                   /* The byte in the file at which the next operation is performed       */
	uint32_t direntry;               /* A reference to the directory entry where the file came from (index) */
	inode_t inode;                 /* A copy of the inode of the file (read from the block device)        */
} filetable_t;

#endif
