#include <fs/fs.h>

/* fs_read - Takes a file descriptor index into the 'oft', a  pointer to a  *
 *           buffer that the function writes data to and a number of bytes  *
 *           to read.                                                       *
 *                                                                          *
 *           'fs_read' reads data starting at the open file's 'head' until  *
 *           it has  copied either 'len' bytes  from the file's  blocks or  *
 *           the 'head' reaches the end of the file.                        *
 *                                                                          *
 * returns - 'fs_read' should return the number of bytes read (either 'len' *
 *           or the  number of bytes  remaining in the file,  whichever is  *
 *           smaller).                                                      */
int32_t read(uint32_t fd, char* buff, uint32_t len) {
	if (fd < 0 || fd >= NUM_FD || oft[fd].state == FSTATE_CLOSED) return -1;
	if (oft[fd].head == oft[fd].inode.size) return 0; /* Nothing left to read. */

	int32_t to_read = len < oft[fd].inode.size - oft[fd].head ? len : oft[fd].inode.size - oft[fd].head;
	int32_t bytes_read = 0;
	while (bytes_read < to_read) {
		/* Find position in block and figure out how much to transfer. */
		int16_t index = oft[fd].head / BDEV_BLOCK_SIZE;
		int16_t offset = oft[fd].head % BDEV_BLOCK_SIZE;
		int16_t transfer = to_read - bytes_read < BDEV_BLOCK_SIZE - offset ?
			to_read - bytes_read : BDEV_BLOCK_SIZE - offset;
		/* Read into buffer at offset bytes_read.*/
		read_bdev(oft[fd].inode.blocks[index], offset, (buff + bytes_read), transfer);
		oft[fd].head += transfer;
		bytes_read += transfer;
	}
	return bytes_read;
}

/* Takes a file descriptor index into the 'oft' and returns that file's size for reading. */
uint32_t get_filesize(uint32_t fd) {
	return oft[fd].inode.size;
}

/* 'inode_write' takes an inode and writes to its blocks directly, used in  * 
 * cases where you only have an inode and not a dirent_t name to open the   * 
 * file with. When writing to directories you know the name of, use 'write' */
int32_t inode_write(inode_t* inode, char* buff, uint32_t offset, uint32_t len) {

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
	if (fd < 0 || fd >= NUM_FD || oft[fd].state == FSTATE_CLOSED) return -1;
	inode_t* inode = &oft[fd].inode;
	uint32_t written = 0;
	uint32_t orig_size = inode->size;
	/* Get index and location. */
	uint16_t index = oft[fd].head / BDEV_BLOCK_SIZE;
	uint16_t offset = oft[fd].head % BDEV_BLOCK_SIZE;
	while (written < len) {
		if (index >= INODE_BLOCKS) return -1; /* Out of blocks. */
		if (inode->blocks[index] == EMPTY) { /* Block at index needs reserved. */
			uint16_t b = find_free_block();
			if (b == -1) return -1; /* No free blocks. */
			setmaskbit_fs(b);
			inode->blocks[index] = b;
		}
		uint32_t to_write = len - written < BDEV_BLOCK_SIZE - offset ? len - written : BDEV_BLOCK_SIZE - offset;
		write_bdev(inode->blocks[index], offset, (buff + written), to_write);
		written += to_write;
		oft[fd].head += to_write;
		++index;
		offset = 0;
	}

	inode->size = oft[fd].head > orig_size ? oft[fd].head : orig_size; /* Overwrites don't add size. */
	write_bdev(inode->id, 0, inode, sizeof(inode_t)); /* write inode */
	write_bdev(BM_BIT, 0, boot_fsd->freemask, boot_fsd->freemasksz); /* write back */
	write_bdev(SB_BIT, 0, boot_fsd, sizeof(*boot_fsd)); /* write super */
	return written;
}

/* fs_write - Takes a file descriptor index  into the 'oft' and an  integer  *
 *            offset.  'fs_seek' moves the current head to match the given   *
 *            offset, bounded by the size of the file.                       *
 *                                                                           *
 *  returns - 'fs_seek' should return the new position of the file head      */
uint32_t seek(uint32_t fd, uint32_t offset, uint32_t relative) {
	/* unimplemented */
	return 0;
}