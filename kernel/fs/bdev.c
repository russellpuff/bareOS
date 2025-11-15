#include <system/memlayout.h>
#include <mm/kmalloc.h>
#include <system/panic.h>
#include <fs/fs.h>
#include <util/string.h>
#include <barelib.h>

/* Initialize the block device by setting its block size/count for future reference. And allocates *
 * the memory for the device itself. This prefers the constants which are meant for the "primary"  *
 * bareOS filesystem, but this allows for setting the device to a custom size. It requires passing *
 * in a "temp" fsd that will be written to the super block and then mounted later, or overwritten  */
uint32_t mk_ramdisk(uint32_t blocksize, uint32_t numblocks, fsystem_t* temp_fsd, bool boot_disk) {
  temp_fsd->device->block_size = (blocksize == NULL ? BDEV_BLOCK_SIZE : blocksize);
  temp_fsd->device->num_blocks = (numblocks == NULL ? BDEV_NUM_BLOCKS : numblocks);
  
  /* If we're not creating the boot disk, we kmalloc space for the second ramdisk.  *
   * As a consequence, we'll be able to find it in the memory backed file by	   *
   * locating the super block and reading it, but restoring it on reboot would be  *
   * a huge pain in the ass and I have no reason to support this right now.        */
  if (boot_disk) {
	  uint64_t requested = (uint64_t)temp_fsd->device->block_size * temp_fsd->device->num_blocks;
	  if (requested > BDEV_BLOCK_SIZE * BDEV_NUM_BLOCKS) {
		  panic("mk_ramdisk was called to create a fresh boot disk larger than the max allotted size\nMax allotted: %lu\nRequested: %lu", 
			  BDEV_BLOCK_SIZE * BDEV_NUM_BLOCKS, requested);
	  }
	  temp_fsd->device->ramdisk = &ramdisk_start;
  }
  else {
	  temp_fsd->device->ramdisk = kmalloc((uint64_t)temp_fsd->device->block_size * temp_fsd->device->num_blocks);
  }

  return (temp_fsd->device->ramdisk == NULL ? -1 : 0);
}

/* Takes a block, offset within, output buffer, and length to read. Checks if the block is valid, *
 * then copies the data from the block to the output buffer.                                      */
uint32_t read_bdev(uint32_t block, uint32_t offset, void* buf, uint32_t len) { 
  if (offset < 0 || offset + len > boot_fsd->device->block_size ||
	  block < 0 || block >= boot_fsd->device->num_blocks ||
	  boot_fsd->device->ramdisk == NULL) {
	return -1;
  }
  memcpy(buf, &(boot_fsd->device->ramdisk[block * boot_fsd->device->block_size]) + offset, len);
  return 0;
}

/* Takes a block, offset within, input buffer, and length to write. Checks if the block is valid, *
 * then copies the data from the buffer to the block.                                             */
uint32_t write_bdev(uint32_t block, uint32_t offset, void* buf, uint32_t len) {
  if (offset + len > boot_fsd->device->block_size || 
	  block >= boot_fsd->device->num_blocks ||
	  boot_fsd->device->ramdisk == NULL) {
	return -1;
  }
  memcpy(&(boot_fsd->device->ramdisk[block * boot_fsd->device->block_size]) + offset, buf, len);
  return 0;
}

/* Simple helper to zero out blocks starting at 'start' with length 'count' */
void bdev_zero_blocks(uint16_t start, uint16_t count) {
	byte* ptr = boot_fsd->device->ramdisk + (start * boot_fsd->device->block_size);
	memset(ptr, 0, boot_fsd->device->block_size * count);
}

/* Simple helper to write the in-memory super back to the hardcopy version on disk */
uint32_t write_super(void) {
	write_bdev(SB_BIT, 0, &boot_fsd->super, sizeof(fsuper_t));
	return 0;
}

/* Helper function allocates a block for the caller */
int32_t allocate_block(void) {
	int32_t blk = bm_findfree();
	if (blk == -1) return FAT_BAD;
	bm_set(blk);
	fat_set(blk, FAT_END);
	bdev_zero_blocks(blk, 1);
	return blk;
}
