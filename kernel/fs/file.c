#include <fs/fs.h>
#include <mm/malloc.h>
#include <lib/string.h>

/* Resolve the directory from the provided filepath, if invalid or the file  *
 * already exists at that path, return an error. Otherwise create a new      *
 * dirent for that directory for this file, create an inode, and assign the  *
 * new file a single block as its head block.                                */
int32_t create(char* filename) {
	/* TEMP: No mechanism exists to resolve a path yet, until then, 
	   assume the file is being created in the current (root) directory */
	return 0;
}

/* Resolve the directory from the provided filepath, if invalid or the  *
 * file does not exist in the target directory, return an error.        *
 * otherwise find a free slot in the fsd's open file table and init for *
 * further operations on the file. Return an fd (index to oft)          */
int32_t open(char* filename) {
	/* TEMP: No mechanism exists to resolve a path yet, until then,
	   assume the target file is in the current (root) directory    */
	int32_t fd = 0;
	return fd;
}

/* Takes an index into the oft and closes the file in the table. *
 * Finishes up by writing the inode back to the inode table and  *
 * any other cleanup work required.                              */
int32_t close(int32_t fd) {
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
	bdev_zero_blocks(ino.head, 1);
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
		char* buff = malloc(sizeof(dir));
		memcpy(buff, &dir, sizeof(dir)); /* seems dumb, maybe change one day? */
		iwrite(&p_ino, buff, p_ino.size, sizeof(dir)); /* TODO: if this returns a value not equal to sizeof(dir), we're out of blocks. */
		write_inode(&p_ino, parent);
		free(buff);
	}

	return dir;
}