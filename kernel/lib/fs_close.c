#include <barelib.h>
#include <fs.h>

extern fsystem_t* fsd;
extern filetable_t oft[NUM_FD];

/*  Modify  the state  of the open  file table to  close  *
 *  the 'fd' index and write the inode back to the block  *
    device.  If the  entry is already closed,  return an  *
 *  error.                                                */
int32 close(int32 fd) {
	if(fd < 0 || fd >= NUM_FD) return -1;
	if(oft[fd].state == FSTATE_CLOSED) return -1;

	/* Write back inode. */
	int16 b = fsd->root_dir.entry[oft[fd].direntry].inode_block;
	if(write_bs(b, 0, &oft[fd].inode, sizeof(inode_t)) == -1) return -1;

	/* Clear the entry and set status to close. */
	oft[fd].state = FSTATE_CLOSED;
	oft[fd].head = 0;
	oft[fd].direntry = EMPTY;
	
	return 0;
}
