#ifndef H_FS
#define H_FS

#include <lib/barelib.h>
#include <fs/format.h>

#define EMPTY -1        /* Used in FS whenever a field's state is undefined or unused */

/* SEEK_START: count from start of file                   */
/* SEEK_END:   count down from the end of the file        */
/* SEEK_HEAD:  move head relative to the current head     */
typedef enum { SEEK_START, SEEK_END, SEEK_HEAD } SEEK_POS;
/* FAT_FREE: the block is unused                          */
/* FAT_END: this is the last block in a fat chain         */
/* FAT_RSVD: this block is reserved for kernel use        */
/* FAT_BAD: returned when an invalid request was made     */
typedef enum { FAT_FREE = 0, FAT_END = -1, FAT_RSVD = -2, FAT_BAD = -3 } FAT_FLAG;

typedef struct { inode_t inode; uint32_t offset; uint32_t in_idx; uint32_t sz; } dir_iter_t;

/* Function prototypes used in the file system */
uint32_t mk_ramdisk(uint32_t, uint32_t, fsystem_t*);      /* Build the block device                      */
uint32_t free_ramdisk(void);                              /* Free resources associated with block device */
uint32_t read_bdev(uint32_t, uint32_t, void*, uint32_t);  /* Read a block from the block device          */
uint32_t write_bdev(uint32_t, uint32_t, void*, uint32_t); /* Write a block to the block device           */
uint32_t write_super(void);                               /* Write the super in memory to the device     */
void bdev_zero_blocks(uint16_t, uint16_t);                /* Zeroes out blocks at a start index          */
int32_t allocate_block(void);                             /* Finds a free block and returns it           */

void bm_set(uint32_t);   /* Mark a block as used      */
void bm_clear(uint32_t); /* Mark a block as unused    */
byte bm_get(uint32_t); /* Get the state of a block  */
int32_t bm_findfree(void); /* Find a free block. */

byte mkfs(uint32_t, uint32_t); /* Create a blank file system               */
byte discover_drive(byte*); /* Try to discover a drive from a ramdisk in memory */
uint32_t mount_fs(drive_t*, const char*);       /* Build the structures for the file system */
uint32_t umount_fs(void);      /* Clear the structures for the file system */

int16_t fat_get(int16_t);          /* Get the next FAT index at the current block. */
int16_t fat_set(int16_t, int16_t); /* Set a value to the specified index.          */

uint16_t in_find_free(void);        /* Find a free entry in the inode table  */
byte write_inode(inode_t, uint16_t); /* Write an in-memory inode to the table */
inode_t get_inode(uint16_t);        /* Get a live copy of the inode at index */

int32_t create(char*, dirent_t);                    /* Create a file and save it to the block device */
int32_t open(char*, dirent_t);                      /* Open a file                                   */
int32_t close(int32_t);                   /* Close a file                                  */
uint32_t iread(inode_t, byte*, uint32_t, uint32_t);
uint32_t iwrite(inode_t*, byte*, uint32_t, uint32_t); /* Write to an inode's blocks         */
uint32_t write(uint32_t, byte*, uint32_t); /* Write to a file                               */
uint32_t read(uint32_t, byte*, uint32_t);    /* Read from file                                */
uint32_t get_filesize(uint32_t);          /* Quick size lookup                             */
dirent_t mk_dir(char*, uint16_t);         /* Create an empty directory                     */
uint8_t resolve_dir(const char*, const dirent_t*, dirent_t*); /* Resolves the lowest directory from a path     */
uint8_t path_to_name(const char*, char*);
int16_t index_to_block(inode_t, uint32_t);
int16_t dir_next(dir_iter_t*, dirent_t*);
uint16_t dir_collect(dirent_t, dirent_t*, uint16_t);
byte dir_open(uint16_t, dir_iter_t*);
void dir_close(dir_iter_t*);
char* dirent_path_expand(dirent_t, char*);
bool dir_child_exists(dirent_t, const char*, dirent_t*);
uint8_t dir_write_entry(dirent_t, dirent_t);

extern fsystem_t* boot_fsd;
extern drv_reg* reg_drives;
extern mount_t* mounted;

#endif
