#include <barelib.h>
#include <fs.h>
#include <string.h>

extern fsystem_t* fsd;
extern filetable_t oft[NUM_FD];

/*  Search for a filename  in the directory, if the file doesn't exist  *
 *  or it is  already  open, return  an error.   Otherwise find a free  *
 *  slot in the open file table and initialize it to the corresponding  *
 *  inode in the root directory.                                        *
 *  'head' is initialized to the start of the file.                     */
int32 open(char* filename) {
	/* Try to find the file by name. */
	int16 slot = -1;
	for(int16 i = 0; i < DIR_SIZE; ++i) {
		if(!fsd->root_dir.entry[i].name[0])  {
			continue;
		}
		if(!strcmp(filename, fsd->root_dir.entry[i].name)) {
			slot = i;
			break;
		}
	}
	if(slot == -1) return -1;

	/* Check if it's open. */
	for (int16 i = 0; i < NUM_FD; ++i)
		if (oft[i].direntry == slot && oft[i].state == FSTATE_OPEN) return -1;

	/* Look for available oft slot. */
	int16 fd = -1;
	for(int i = 0; i < NUM_FD; ++i) {
		if(oft[i].state == FSTATE_CLOSED) {
			fd = i;
			break;
		}
	}
	if(fd == -1) return -1;

	/* Read inode. */
	inode_t inode;
	if(read_bs(fsd->root_dir.entry[slot].inode_block, 0, &inode, sizeof(inode_t)) == -1) return -1;

	/* Populate oft slot with info. */
	oft[fd].state = FSTATE_OPEN;
	oft[fd].head = 0;
	oft[fd].direntry = slot;
	oft[fd].inode = inode;

	return fd;
}
