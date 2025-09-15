#ifndef H_FS
#define H_FS

#include <lib/barelib.h>
#include <fs/format.h>

#define EMPTY -1        /* Used in FS whenever a field's state is undefined or unused */

#define SB_BIT 0        /* Alias for the super block index                            */
#define BM_BIT 1        /* Alias for the bitmask block index                          */

/* SEEK_START: count from start of file                   */
/* SEEK_END:   count down from the end of the file        */
/* SEEK_HEAD:  move head relative to the current head     */
typedef enum { SEEK_START, SEEK_END, SEEK_HEAD } SEEK_POS;
/* FAT_FREE: the block is unused                          */
/* FAT_END: this is the last block in a fat chain         */
/* FAT_RSVD: this block is reserved for kernel use        */
/* FAT_BAD: returned when an invalid request was made     */
typedef enum { FAT_FREE = 0, FAT_END = -1, FAT_RSVD = -2, FAT_BAD = -3 } FAT_FLAG;

/* Function prototypes used in the file system */
uint32_t mk_ramdisk(uint32_t, uint32_t);                  /* Build the block device                      */
uint32_t free_ramdisk(void);                              /* Free resources associated with block device */
uint32_t read_bdev(uint32_t, uint32_t, void*, uint32_t);  /* Read a block from the block device          */
uint32_t write_bdev(uint32_t, uint32_t, void*, uint32_t); /* Write a block to the block device           */

void bm_set(uint32_t);   /* Mark a block as used      */
void bm_clear(uint32_t); /* Mark a block as unused    */
byte bm_get(uint32_t); /* Get the state of a block  */
int32_t bm_findfree(void); /* Find a free block. */

void mkfs(void);          /* Save the super block and bitmask for the FS */
uint32_t mount_fs(void);  /* Build the structures for the file system    */
uint32_t umount_fs(void); /* Clear the structures for the file system    */

int16_t fat_get(int16_t); /* Get the next FAT index at the current block. */
int16_t fat_set(int16_t, int16_t); /* Set a value to the specified index. */

int32_t create(char*);                    /* Create a file and save it to the block device */
int32_t open(char*);                      /* Open a file                                   */
int32_t close(int32_t);                   /* Close a file                                  */
int32_t write(uint32_t, char*, uint32_t); /* Write to a file                               */
int32_t read(uint32_t,char*,uint32_t);    /* Read from file                                */
uint32_t get_filesize(uint32_t);          /* Quick size lookup                             */

extern fsystem_t* fsd;

#endif
