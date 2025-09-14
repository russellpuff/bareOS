#include <system/interrupts.h>
#include <lib/barelib.h>
#include <lib/string.h>
#include <mm/malloc.h>
#include <fs/fs.h>


static bdev_t ramdisk;             /* After initialization, contains metadata about the block device */
static byte* ramfs_blocks = NULL;  /* A pointer to the actual memory used as the block device        */


uint32_t mk_ramdisk(uint32_t blocksize, uint32_t numblocks) {                       /*                               */
  ramdisk.blocksz = (blocksize == NULL ? MDEV_BLOCK_SIZE : blocksize);        /*  Initialize the block device  */ 
  ramdisk.nblocks = (numblocks == NULL ? MDEV_NUM_BLOCKS : numblocks);        /*  This  sets  the block  size  */
  ramfs_blocks = malloc(ramdisk.blocksz * ramdisk.nblocks);                   /*  and block count for  future  */
                                                                              /*  reference.   And  allocates  */  
  return (ramfs_blocks == (void*)-1 ? -1 : 0);                                /*  the memory  for the  device  */
}                                                                             /*  itself.                      */

bdev_t bdev_getstats(void) {   /*                                                            */
  return ramdisk;         /*  External accessor function for the block device metadata  */
}                         /*                                                            */

uint32_t free_ramdisk(void) {     /*                                        */
  if (ramfs_blocks == NULL) {   /*                                        */
    return -1;                  /*                                        */
  }                             /*  Free memory used by the block device  */
  free(ramfs_blocks);           /*                                        */
  return 0;                     /*                                        */
}

uint32_t read_bdev(uint32_t block, uint32_t offset, void* buf, uint32_t len) {    /*                                   */
  if (offset < 0 || offset + len > ramdisk.blocksz ||                   /*  Check if the block is valid      */
      block < 0 || block >= ramdisk.nblocks ||                          /*  if not valid, restore interrupts */
      ramfs_blocks == NULL) {                                           /*  and return error.                */
    return -1;                                                          /*                                   */
  }                                                                     /*                                   */
  memcpy(buf, &(ramfs_blocks[block * ramdisk.blocksz]) + offset, len);  /*  Copy the data from the block to  */
  return 0;                                                             /*  the output buffer.               */
}                                                                       /*                                   */

uint32_t write_bdev(uint32_t block, uint32_t offset, void* buf, uint32_t len) {   /*                                   */
  if (offset < 0 || offset + len > ramdisk.blocksz ||                   /*  Check if the block is valid      */
      block < 0 || block >= ramdisk.nblocks ||                          /*  if not valid, restore interrupts */
      ramfs_blocks == NULL) {                                           /*  and return error.                */
    return -1;                                                          /*                                   */
  }                                                                     /*                                   */
  memcpy(&(ramfs_blocks[block * ramdisk.blocksz]) + offset, buf, len);  /*  Copy the data from the buffer    */
  return 0;                                                             /*  to the block.                    */
}                                                                       /*                                   */
