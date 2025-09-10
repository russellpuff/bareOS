#include <lib/barelib.h>
#include <fs/fs.h>

/* Helper function to find the first free block. */
int32_t find_free_block(void) {
	/* Start at 2 to skip super and bitmask blocks. */
	for(uint16_t b = 2; b < fsd->device.nblocks; ++b) {
		if(getmaskbit_fs(b) == 0) return b;
	}
	return -1;
}

/* fs_write - Takes a file descriptor index into the 'oft', a  pointer to a  *
 *            buffer  that the  function reads data  from and the number of  *
 *            bytes to copy from the buffer to the file.                     *
 *                                                                           *
 *            'fs_write' reads data from the 'buff' and copies it into the   *
 *            file  'blocks' starting  at the 'head'.  The  function  will   *
 *            allocate new blocks from the block device as needed to write   *
 *            data to the file and assign them to the file's inode.          *
 *                                                                           *
 *  returns - 'fs_write' should return the number of bytes written to the    *
 *            file.                                                          */
int32_t write(uint32_t fd, char* buff, uint32_t len) {
	if(fd < 0 || fd >= NUM_FD || oft[fd].state == FSTATE_CLOSED) return -1;
	inode_t* inode = &oft[fd].inode;
	uint32_t written = 0;
	uint32_t orig_size = inode->size;
	/* Get index and location. */
	uint16_t index = oft[fd].head / MDEV_BLOCK_SIZE;
	uint16_t offset = oft[fd].head % MDEV_BLOCK_SIZE;
	while(written < len) {
		if(index >= INODE_BLOCKS) return -1; /* Out of blocks. */
		if(inode->blocks[index] == EMPTY) { /* Block at index needs reserved. */
			uint16_t b = find_free_block();
			if(b == -1) return -1; /* No free blocks. */
			setmaskbit_fs(b);
			inode->blocks[index] = b;
		}
		uint32_t to_write = len - written < MDEV_BLOCK_SIZE - offset ? len - written : MDEV_BLOCK_SIZE - offset;
		write_bs(inode->blocks[index], offset, (buff + written), to_write);
		written += to_write;
		oft[fd].head += to_write;
		++index;
		offset = 0;
	}

	inode->size = oft[fd].head > orig_size ? oft[fd].head : orig_size; /* Overwrites don't add size. */
	write_bs(inode->id, 0, inode, sizeof(inode_t)); /* write inode */
	write_bs(BM_BIT, 0, fsd->freemask, fsd->freemasksz); /* write back */
	write_bs(SB_BIT, 0, fsd, sizeof(*fsd)); /* write super */
  	return written;
}
