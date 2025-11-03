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
	int32_t bytes_read = 0;
	return bytes_read;
}

/* Takes a file descriptor index into the 'oft' and returns that file's size for reading. */
uint32_t get_filesize(uint32_t fd) {
	return boot_fsd->oft[fd].inode.size;
}

/* 'iwrite' takes an inode and writes to its blocks directly, used in  * 
 * cases where you only have an inode and not a dirent_t name to open the   * 
 * file with. When writing to directories you know the name of, use 'write' 
 * Returns the number of bytes written (even if truncated due to lack of storage) */
int32_t iwrite(inode_t* inode, char* buff, uint32_t offset, uint32_t len) {
	return 0;
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
	int32_t written = 0;
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