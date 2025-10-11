#include <fs/fs.h>
#include <lib/string.h>

/*  Search for 'filename' in the root directory.  If the  *
 *  file exists,  returns an error.  Otherwise, create a  *
 *  new file  entry in the  root directory, a llocate an  *
 *  unused  block in the block  device and  assign it to  *
 *  the new file.                                         */
int32_t create(char* filename) {
	if (fsd->root_dir.numentries == DIR_SIZE) return -1;

	/* Try to find a slot. */
	for (uint32_t i = 0; i < fsd->root_dir.numentries; ++i) {
		if (!strcmp(filename, fsd->root_dir.entry[i].name))
			return -1;
	}
	/* Use next available slot (no way to delete files so whatever). */
	int32_t slot = fsd->root_dir.numentries;

	/* Find bit to use. */
	uint32_t b = 0;
	for (; b < fsd->device.nblocks; ++b)
		if (!getmaskbit_fs(b)) break;
	if (b == fsd->device.nblocks) return -1;

	/* Copy in filename. */
	memset(fsd->root_dir.entry[slot].name, '\0', (uint64_t)FILENAME_LEN);
	byte ncopy = 0;
	while (ncopy < (FILENAME_LEN - 1) && filename[ncopy] != '\0') ++ncopy;
	memcpy(fsd->root_dir.entry[slot].name, filename, ncopy);

	for (uint32_t n = 0; filename[n] && n < FILENAME_LEN - 1; ++n)
		fsd->root_dir.entry[slot].name[n] = filename[n];

	fsd->root_dir.entry[slot].inode_block = b;
	/* Construct inode. */
	inode_t inode;
	inode.id = b;
	inode.size = 0;
	for (uint16_t i = 0; i < INODE_BLOCKS; ++i)
		inode.blocks[i] = EMPTY;
	if (write_bs(b, 0, &inode, sizeof(inode_t)) == -1) return -1;

	++fsd->root_dir.numentries;
	setmaskbit_fs(b);

	if (write_bs(BM_BIT, 0, fsd->freemask, fsd->freemasksz) == -1) return -1; /* write back */
	if (write_bs(SB_BIT, 0, fsd, sizeof(fsystem_t)) == -1) return -1; /* write super */
	return 0;
}

/*  Search for a filename  in the directory, if the file doesn't exist  *
 *  or it is  already  open, return  an error.   Otherwise find a free  *
 *  slot in the open file table and initialize it to the corresponding  *
 *  inode in the root directory.                                        *
 *  'head' is initialized to the start of the file.                     */
int32_t open(char* filename) {
	/* Try to find the file by name. */
	int16_t slot = -1;
	for (int16_t i = 0; i < DIR_SIZE; ++i) {
		if (!fsd->root_dir.entry[i].name[0]) {
			continue;
		}
		if (!strcmp(filename, fsd->root_dir.entry[i].name)) {
			slot = i;
			break;
		}
	}
	if (slot == -1) return -1;

	/* Check if it's open. */
	for (int16_t i = 0; i < NUM_FD; ++i)
		if (oft[i].direntry == slot && oft[i].state == FSTATE_OPEN) return -1;

	/* Look for available oft slot. */
	int16_t fd = -1;
	for (uint32_t i = 0; i < NUM_FD; ++i) {
		if (oft[i].state == FSTATE_CLOSED) {
			fd = i;
			break;
		}
	}
	if (fd == -1) return -1;

	/* Read inode. */
	inode_t inode;
	if (read_bs(fsd->root_dir.entry[slot].inode_block, 0, &inode, sizeof(inode_t)) == -1) return -1;

	/* Populate oft slot with info. */
	oft[fd].state = FSTATE_OPEN;
	oft[fd].head = 0;
	oft[fd].direntry = slot;
	oft[fd].inode = inode;

	return fd;
}

/*  Modify  the state  of the open  file table to  close  *
 *  the 'fd' index and write the inode back to the block  *
	device.  If the  entry is already closed,  return an  *
 *  error.                                                */
int32_t close(int32_t fd) {
	if (fd < 0 || fd >= NUM_FD) return -1;
	if (oft[fd].state == FSTATE_CLOSED) return -1;

	/* Write back inode. */
	int16_t b = fsd->root_dir.entry[oft[fd].direntry].inode_block;
	if (write_bs(b, 0, &oft[fd].inode, sizeof(inode_t)) == -1) return -1;

	/* Clear the entry and set status to close. */
	oft[fd].state = FSTATE_CLOSED;
	oft[fd].head = 0;
	oft[fd].direntry = EMPTY;

	return 0;
}