#include <system/interrupts.h>
#include <lib/barelib.h>
#include <lib/string.h>
#include <mm/malloc.h>
#include <fs/fs.h>

/* Initialize the block device by setting its block size/count for future reference. And allocates *
 * the memory for the device itself. This prefers the constants which are meant for the "primary"  *
 * bareOS filesystem, but this allows for setting the device to a custom size. Since it assumes    *
 * the fsd-associated bdev, you can't make additional devices yet.                                 */
uint32_t mk_ramdisk(uint32_t blocksize, uint32_t numblocks) {
  fsd->device.block_size = (blocksize == NULL ? BDEV_BLOCK_SIZE : blocksize);
  fsd->device.num_blocks = (numblocks == NULL ? BDEV_NUM_BLOCKS : numblocks);
  fsd->device.ramdisk = malloc((uint64_t)fsd->device.block_size * fsd->device.num_blocks);

  return (fsd->device.ramdisk == (void*)-1 ? -1 : 0);
}

/* Frees the memory used by the block device. Still assumes the fsd has the only bdev. */
uint32_t free_ramdisk(void) {
  if (fsd->device.ramdisk == NULL) {
    return -1;
  }
  free(fsd->device.ramdisk);
  return 0;
}

/* Takes a block, offset within, output buffer, and length to read. Checks if the block is valid, *
 * then copies the data from the block to the output buffer.                                      */
uint32_t read_bdev(uint32_t block, uint32_t offset, void* buf, uint32_t len) { 
  if (offset < 0 || offset + len > fsd->device.block_size ||
      block < 0 || block >= fsd->device.num_blocks ||
      fsd->device.ramdisk == NULL) {
    return -1;
  }
  memcpy(buf, &(fsd->device.ramdisk[block * fsd->device.block_size]) + offset, len);
  return 0;
}

/* Takes a block, offset within, input buffer, and length to write. Checks if the block is valid, *
 * then copies the data from the buffer to the block.                                             */
uint32_t write_bdev(uint32_t block, uint32_t offset, void* buf, uint32_t len) {
  if (offset < 0 || offset + len > fsd->device.block_size ||
      block < 0 || block >= fsd->device.num_blocks ||
      fsd->device.ramdisk == NULL) {
    return -1;
  }
  memcpy(&(fsd->device.ramdisk[block * fsd->device.block_size]) + offset, buf, len);
  return 0;
}
