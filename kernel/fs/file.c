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
		if (!bm_get(b)) break;
	if (b == fsd->device.nblocks) return -1;

	/* Copy in filename. */
	memset(fsd->root_dir.entry[slot].name, '\0', (uint64_t)FILENAME_LEN);
	byte ncopy = 0;
	while (ncopy < (FILENAME_LEN - 1) && filename[ncopy] != '\0') ++ncopy;
	memcpy(fsd->root_dir.entry[slot].name, filename, ncopy);

	for (byte n = 0; filename[n] && n < FILENAME_LEN - 1; ++n)
		fsd->root_dir.entry[slot].name[n] = filename[n];

	fsd->root_dir.entry[slot].inode_block = b;
	/* Construct inode. */
	inode_t inode;
	inode.id = b;
	inode.size = 0;
	for (uint16_t i = 0; i < INODE_BLOCKS; ++i)
		inode.blocks[i] = EMPTY;
	if (write_bdev(b, 0, &inode, sizeof(inode_t)) == -1) return -1;

	++fsd->root_dir.numentries;
	bm_set(b);

	if (write_bdev(BM_BIT, 0, fsd->freemask, fsd->freemasksz) == -1) return -1; /* write back */
	if (write_bdev(SB_BIT, 0, fsd, sizeof(fsystem_t)) == -1) return -1; /* write super */
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
	for (byte i = 0; i < NUM_FD; ++i) {
		if (oft[i].state == FSTATE_CLOSED) {
			fd = i;
			break;
		}
	}
	if (fd == -1) return -1;

	/* Read inode. */
	inode_t inode;
	if (read_bdev(fsd->root_dir.entry[slot].inode_block, 0, &inode, sizeof(inode_t)) == -1) return -1;

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
	if (write_bdev(b, 0, &oft[fd].inode, sizeof(inode_t)) == -1) return -1;

	/* Clear the entry and set status to close. */
	oft[fd].state = FSTATE_CLOSED;
	oft[fd].head = 0;
	oft[fd].direntry = EMPTY;

	return 0;
}

dirent_t get_dot_entry(uint16_t inode, const char* name) {
	dirent_t dot;
	dot.inode = inode;
	uint32_t len = strlen(name);
	memcpy(dot.name, name, len);
	dot.name[len] = '\0';
	dot.type = DIR;
	return dot;
}

dirent_t mk_dir(char* name, uint16_t parent) {
	/* Make dirent */
	dirent_t dir;
	dir.type = DIR;
	dir.inode = in_find_free(); /* TODO: Check for invalid. */
	uint32_t len = strlen(name);
	memcpy(&dir.name, name, len);
	dir.name[len] = '\0';

	/* Make inode */
	inode_t ino;
	ino.head = bm_findfree();
	bm_set(ino.head);
	fat_set(ino.head, FAT_END);
	bdev_zero_blocks(&fsd->device, ino.head, 1);
	ino.parent = parent;
	ino.type = DIR;
	ino.size = sizeof(dirent_t) * 2;
	ino.modified = 0; // Unused for now.
	write_inode(&ino, dir.inode); /* TODO: Check for invalid. */

	/* Make default subdirectories */
	dirent_t self_dot = get_dot_entry(dir.inode, ".");
	dirent_t parent_dot = get_dot_entry(parent, "..");
	write_bdev(ino.head, 0, &self_dot, sizeof(dirent_t));
	write_bdev(ino.head, sizeof(dirent_t), &parent_dot, sizeof(dirent_t));

	if (dir.inode != ino.parent) {
		/* Write dirent to parent if self isn't parent. */
		inode_t p_ino = get_inode(parent);
		inode_write(&p_ino, &dir, p_ino.size, sizeof(dir)); /* TODO: if this returns a value not equal to sizeof(dir), we're out of blocks. */
		write_inode(&p_ino, parent);
	}

	return dir;
}