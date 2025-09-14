#ifndef H_FS
#define H_FS

#include <lib/barelib.h>
#include <fs/format.h>

#define EMPTY -1        /* Used in FS whenever a field's state is undefined or unused */

#define SB_BIT 0        /* Alias for the super block index                            */
#define BM_BIT 1        /* Alias for the bitmask block index                          */

#define SEEK_START 0    /* Used in `seek`, count from start of file                   */
#define SEEK_END   1    /* Used in `seek`, count down from the end of the file        */
#define SEEK_HEAD  2    /* Used in `seek`, move head relative to the current head     */

/* Function prototypes used in the file system */
bdev_t bdev_getstats(void);                               /* Retreive the embedded block dev info        */
uint32_t mk_ramdisk(uint32_t, uint32_t);                  /* Build the block device                      */
uint32_t free_ramdisk(void);                              /* Free resources associated with block device */
uint32_t read_bdev(uint32_t, uint32_t, void*, uint32_t);  /* Read a block from the block device          */
uint32_t write_bdev(uint32_t, uint32_t, void*, uint32_t); /* Write a block to the block device           */

void   setmaskbit_fs(uint32_t);   /* Mark a block as used      */
void   clearmaskbit_fs(uint32_t); /* Mark a block as unused    */
uint32_t getmaskbit_fs(uint32_t); /* Get the state of a block  */

void mkfs(void);          /* Save the super block and bitmask for the FS */
uint32_t mount_fs(void);  /* Build the structures for the file system    */
uint32_t umount_fs(void); /* Clear the structures for the file system    */

int32_t create(char*);                    /* Create a file and save it to the block device */
int32_t open(char*);                      /* Open a file                                   */
int32_t close(int32_t);                   /* Close a file                                  */
int32_t write(uint32_t, char*, uint32_t); /* Write to a file                               */
int32_t read(uint32_t,char*,uint32_t);    /* Read from file                                */
uint32_t get_filesize(uint32_t);          /* Quick size lookup                             */

extern fsystem_t* fsd;

#endif
