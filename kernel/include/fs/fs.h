#ifndef H_FS
#define H_FS

#include <lib/barelib.h>

#define EMPTY     -1            /* Used in FS whenever a field's state is undefined or unused */

#define SB_BIT 0                /* Alias for the super block index                            */
#define BM_BIT 1                /* Alias for the bitmask block index                          */

#define FILENAME_LEN 32         /* Maximum length of a filename in the FS                     */
#define DIR_SIZE     16         /* Maximum number of files referenced in the root direcotry   */

#define INODE_BLOCKS    64      /* Maximum number of blocks associated with each file inode   */
#define MDEV_BLOCK_SIZE 2048    /* Size of each block in bytes                                */
#define MDEV_NUM_BLOCKS 512     /* Number of blocks in the block device                       */

#define FSTATE_CLOSED 0         /* Used when opening and closing files to indicate the state  */
#define FSTATE_OPEN   1         /*     of the slot in the open file table.                    */
#define NUM_FD       10         /* Number of slots in the open file table                     */

#define SEEK_START 0            /* Used in `fs_seek`, count from start of file                */
#define SEEK_END   1            /* Used in `fs_seek`, count down from the end of the file     */
#define SEEK_HEAD  2            /* Used in `fs_seek`, move head relative to the current head  */


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

/* Function prototypes used in the file system */
bdev_t bs_getstats(void);                       /* Retreive the embedded block dev info        */
uint32_t mk_ramdisk(uint32_t, uint32_t);              /* Build the block device                      */
uint32_t free_ramdisk(void);                      /* Free resources associated with block device */
uint32_t read_bs(uint32_t, uint32_t, void*, uint32_t);  /* Read a block from the block device          */
uint32_t write_bs(uint32_t, uint32_t, void*, uint32_t); /* Write a block to the block device           */

void   setmaskbit_fs(uint32_t);                   /* Mark a block as used                        */
void   clearmaskbit_fs(uint32_t);                 /* Mark a block as unused                      */
uint32_t getmaskbit_fs(uint32_t);                   /* Get the state of a block                    */

void mkfs(void);                                /* Save the super block and bitmask for the FS */
uint32_t mount_fs(void);                          /* Build the structures for the file system    */
uint32_t umount_fs(void);                         /* Clear the structures for the file system    */

int32_t create(char*);                            /* Create a file and save it to the block device */
int32_t open(char*);                              /* Open a file                                   */
int32_t close(int32_t);                             /* Close a file                                  */
int32_t write(uint32_t, char*, uint32_t);               /* Write to a file                               */
int32_t read(uint32_t,char*,uint32_t);                /* Read from file                                */
uint32_t get_filesize(uint32_t);

extern fsystem_t* fsd;
extern filetable_t oft[NUM_FD];

#endif
