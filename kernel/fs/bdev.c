#include <system/interrupts.h>
#include <lib/barelib.h>
#include <lib/string.h>
#include <mm/malloc.h>
#include <fs/fs.h>

/* Initialize the block device by setting its block size/count for future reference. And allocates *
 * the memory for the device itself. This prefers the constants which are meant for the "primary"  *
 * bareOS filesystem, but this allows for setting the device to a custom size. It requires passing *
 * in a "temp" fsd that will be written to the super block and then mounted later, or overwritten  */
uint32_t mk_ramdisk(uint32_t blocksize, uint32_t numblocks, fsystem_t* temp_fsd) {
  temp_fsd->device.block_size = (blocksize == NULL ? BDEV_BLOCK_SIZE : blocksize);
  temp_fsd->device.num_blocks = (numblocks == NULL ? BDEV_NUM_BLOCKS : numblocks);
  temp_fsd->device.ramdisk = malloc((uint64_t)temp_fsd->device.block_size * temp_fsd->device.num_blocks);

  return (temp_fsd->device.ramdisk == (void*)-1 ? -1 : 0);
}

/* Frees the memory used by the block device. Still assumes the fsd has the only bdev. */
uint32_t free_ramdisk(void) {
  if (boot_fsd->device.ramdisk == NULL) {
    return -1;
  }
  free(boot_fsd->device.ramdisk);
  return 0;
}

/* Takes a block, offset within, output buffer, and length to read. Checks if the block is valid, *
 * then copies the data from the block to the output buffer.                                      */
uint32_t read_bdev(uint32_t block, uint32_t offset, void* buf, uint32_t len) { 
  if (offset < 0 || offset + len > boot_fsd->device.block_size ||
      block < 0 || block >= boot_fsd->device.num_blocks ||
      boot_fsd->device.ramdisk == NULL) {
    return -1;
  }
  memcpy(buf, &(boot_fsd->device.ramdisk[block * boot_fsd->device.block_size]) + offset, len);
  return 0;
}

/* Takes a block, offset within, input buffer, and length to write. Checks if the block is valid, *
 * then copies the data from the buffer to the block.                                             */
uint32_t write_bdev(uint32_t block, uint32_t offset, void* buf, uint32_t len) {
  if (offset < 0 || offset + len > boot_fsd->device.block_size ||
      block < 0 || block >= boot_fsd->device.num_blocks ||
      boot_fsd->device.ramdisk == NULL) {
    return -1;
  }
  memcpy(&(boot_fsd->device.ramdisk[block * boot_fsd->device.block_size]) + offset, buf, len);
  return 0;
}

/* Simple helper to zero out blocks starting at 'start' with length 'count' */
void bdev_zero_blocks(uint16_t start, uint16_t count) {
    byte* ptr = boot_fsd->device.ramdisk + (start * boot_fsd->device.block_size);
    memset(ptr, 0x0, boot_fsd->device.block_size * start);
}