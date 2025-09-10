#include <barelib.h>
#include <fs.h>

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
	if(fd < 0 || fd >= NUM_FD || oft[fd].state == FSTATE_CLOSED) return -1;
	if(oft[fd].head == oft[fd].inode.size) return 0; /* Nothing left to read. */

	int32_t to_read = len < oft[fd].inode.size - oft[fd].head ? len : oft[fd].inode.size -  oft[fd].head;
	int32_t bytes_read = 0;
	while(bytes_read < to_read) {
		/* Find position in block and figure out how much to transfer. */
		int16_t index = oft[fd].head / MDEV_BLOCK_SIZE;
		int16_t offset = oft[fd].head % MDEV_BLOCK_SIZE;
		int16_t transfer = to_read - bytes_read < MDEV_BLOCK_SIZE - offset ? 
			to_read - bytes_read : MDEV_BLOCK_SIZE - offset;
		/* Read into buffer at offset bytes_read.*/
		read_bs(oft[fd].inode.blocks[index], offset, (buff + bytes_read), transfer);
		oft[fd].head += transfer;
		bytes_read += transfer;
	}
	return bytes_read;
}
