#include <fs/fs.h>
#include <mm/malloc.h>
#include <lib/string.h>

/* Resolve the directory from the provided filepath, if invalid or the file  *
 * already exists at that path, return an error. Otherwise create a new      *
 * dirent for that directory for this file, create an inode, and assign the  *
 * new file a single block as its head block.                                */
int32_t create(const char* path, dirent_t cwd) {
	if (path == NULL) return -1; /* Invalid path */

	dirent_t parent;
	uint8_t res = resolve_dir(path, cwd, &parent);
	/* TODO: log the reason or something idk */
	if (res != 0) return -1; /* Invalid path */

	char filename[FILENAME_LEN];
	res = path_to_name(path, filename);
	if (res != 2) return -2; /* Invalid filename, todo: differentiate between fail reasons */

	if (dir_child_exists(parent, filename, NULL)) return -3; /* File exists */

	/* Create entry */
	dirent_t entry;
	memset(&entry, 0, sizeof(dirent_t));
	uint32_t len = strlen(filename);
	memcpy(entry.name, filename, len);
	entry.name[len] = '\0';
	entry.type = EN_FILE;

	/* Create inode */
	entry.inode = in_find_free();
	if (entry.inode == IN_ERR) return -4; /* Error creating entry */
	inode_t inode;
	memset(&inode, 0, sizeof(inode_t));
	inode.type = EN_FILE;
	inode.parent = parent.inode;
	inode.size = 0;
	inode.modified = 0;

	/* Allocate inode block */
	inode.head = allocate_block();
	if (inode.head == FAT_BAD) {
		memset(&inode, 0, sizeof(inode_t));
		inode.type = EN_FREE;
		write_inode(inode, entry.inode);
		return -4; /* Error creating entry */
	}
	write_inode(inode, entry.inode);

	/* Write entry */
	dir_write_entry(parent, entry);

	return 0;
}

/* Resolve the directory from the provided filepath, if invalid or the  *
 * file does not exist in the target directory, return an error.        *
 * otherwise find a free slot in the fsd's open file table and init for *
 * further operations on the file. Return an fd (index to oft)          */
int32_t open(const char* path, FILE* f, dirent_t cwd) {
	if (path == NULL) return -1;
	
	dirent_t parent;
	uint8_t res = resolve_dir(path, cwd, &parent);
	/* TODO: log the reason or something idk */
	if (res != 0) return -1; /* Invalid path */

	char filename[FILENAME_LEN];
	res = path_to_name(path, filename);
	if (res != 2) return -2; /* Invalid filename, todo: differentiate between fail reasons */

	dirent_t file;
	if (!dir_child_exists(parent, filename, &file)) return -3; /* File doesn't exist */
	if (file.type == EN_DIR) return -4; /* File is a directory */

	for (uint8_t i = 0; i < OFT_MAX; ++i) {
		if (boot_fsd->oft[i].state == OPEN && boot_fsd->oft[i].in_index == file.inode) return -5; /* File in use by other process */
	}

	uint8_t fd = (uint8_t)-1;
	for (uint8_t i = 0; i < OFT_MAX; ++i) {
		if (boot_fsd->oft[i].state == CLOSED) fd = i;
	}
	if (fd == (uint8_t)-1) return -5; /* oft error */

	filetable_t* entry = &boot_fsd->oft[fd];
	
	entry->state = OPEN;
	entry->mode = RDWR; /* TODO: open options */
	entry->in_index = file.inode;
	entry->in_dirty = false;
	entry->curr_index = 0;

	f->inode = get_inode(file.inode);
	f->fd = fd;
	entry->inode = &f->inode;

	return 0;
}

/* Takes an index into the oft and closes the file in the table. *
 * Finishes up by writing the inode back to the inode table and  *
 * any other cleanup work required.                              */
int32_t close(FILE* f) {
	if (f->fd < 0 || f->fd >= OFT_MAX) return -1;

	filetable_t* entry = &boot_fsd->oft[f->fd];
	if (entry->state != OPEN) return -1;

	if (entry->in_dirty) {
		write_inode(*entry->inode, entry->in_index);
	}

	entry->state = CLOSED;
	entry->mode = RD_ONLY;
	entry->in_dirty = false;
	entry->curr_index = 0;
	entry->inode = NULL;

	return 0;
}

dirent_t get_dot_entry(uint16_t inode, const char* name) {
	dirent_t dot;
	dot.inode = inode;
	uint32_t len = strlen(name);
	memcpy(dot.name, name, len);
	dot.name[len] = '\0';
	dot.type = EN_DIR;
	return dot;
}

int8_t mk_dir(const char* path, dirent_t cwd, dirent_t* out) {
	if (path == NULL || out == NULL) return -1;
	
	dirent_t parent;
	uint8_t res = resolve_dir(path, cwd, &parent);
	/* TODO: log the reason or something idk */
	if (res != 0) return -1; /* Invalid path */

	char dir_name[FILENAME_LEN];
	res = path_to_name(path, dir_name);
	if (res != 1 && res != 2) return -2; /* Invalid directory name */
	if (!strcmp(dir_name, parent.name)) return -3; /* Directory exists, resolve_dir resolved the preexisting target dir */

	out->type = EN_DIR;
	out->inode = in_find_free();
	if (out->inode == IN_ERR) return -4; /* Error creating entry */
	uint32_t len = strlen(dir_name);
	memcpy(out->name, dir_name, len);
	out->name[len] = '\0';

	inode_t inode;
	memset(&inode, 0, sizeof(inode));
	inode.type = EN_DIR;
	inode.parent = parent.inode;
	/* Allocate inode block */
	inode.head = allocate_block();
	if (inode.head == FAT_BAD) {
		memset(&inode, 0, sizeof(inode_t));
		inode.type = EN_FREE;
		write_inode(inode, out->inode);
		return -4; /* Error creating entry */
	}
	write_inode(inode, out->inode);

	/* Make default subdirectories */
	dirent_t self_dot = get_dot_entry(out->inode, ".");
	dirent_t parent_dot = get_dot_entry(parent.inode, "..");
	dir_write_entry(*out, self_dot);
	dir_write_entry(*out, parent_dot);

	/* Write dirent to parent if self isn't parent. */
	if (out->inode != parent.inode) dir_write_entry(parent, *out);

	return 0;
}

uint8_t dir_open(uint16_t inode_idx, dir_iter_t* it) {
	if (it == NULL) return 1;
	inode_t inode = get_inode(inode_idx);
	if (inode.type != EN_DIR) return 1;
	it->inode = inode;
	it->in_idx = inode_idx;
	it->offset = 0;
	it->sz = inode.size;
	return 0;
}

/* Not that useful for now */
void dir_close(dir_iter_t* it) {
	if (it == NULL) return;
	memset(it, 0, sizeof(*it));
}

/* Helper removes a dirent_t from a parent directory by overwriting 
   the matching slot with a tombstone entry. */
static int8_t dir_remove_entry(dirent_t parent, dirent_t target) {
	if (parent.type != EN_DIR) return -1; /* Parent isn't a directory */

	dir_iter_t it;
	if (dir_open(parent.inode, &it) != 0) return -1; /* Couldn't open directory */

	dirent_t entry;
	while (dir_next(&it, &entry) == 1) {
		if (entry.inode != target.inode) continue;
		if (strcmp(entry.name, target.name)) continue;

		uint32_t offset = it.offset - sizeof(dirent_t);
		dirent_t tombstone;
		memset(&tombstone, 0, sizeof(tombstone));
		tombstone.type = EN_FREE;

		inode_t parent_inode = get_inode(parent.inode);
		if (iwrite(&parent_inode, (byte*)&tombstone, offset, sizeof(dirent_t)) != sizeof(dirent_t)) {
			dir_close(&it);
			return -2; /* Failed to write to parent inode blocks */
		}
		write_inode(parent_inode, parent.inode);
		dir_close(&it);
		return 0;
	}

	dir_close(&it);
	return -3; /* Failed to find child */
}

/* Helper releases all FAT blocks owned by an inode, clears the block usage bit, and marks the
 * inode table entry as free so the inode can be reused later. */
static void inode_release(uint16_t inode_idx) {
	inode_t inode = get_inode(inode_idx);
	int16_t block = inode.head;
	while (block >= 0) {
		int16_t next = fat_get(block);
		bm_clear(block);
		fat_set(block, FAT_FREE);
		if (next == FAT_END) break;
		block = next;
	}

	memset(&inode, 0, sizeof(inode));
	inode.type = EN_FREE;
	inode.head = IN_ERR;
	inode.parent = IN_ERR;
	write_inode(inode, inode_idx);
}

/* Delete an empty directory */
int32_t rm_dir(const char* path, dirent_t cwd) {
	if (path == NULL) return -1;

	dirent_t container;
	uint8_t status = resolve_dir(path, cwd, &container);
	if (status != 0) return -1; /* Invalid path */

	char dirname[FILENAME_LEN];
	status = path_to_name(path, dirname);
	if (status == 0) return -2; /* Invalid directory name */
	if (status == 3) return -3; /* Root directory is immutable */
	if (status == 4 || status == 5) return -2; /* Dot entries not valid targets */

	dirent_t parent = container;
	dirent_t target;
	bool found = dir_child_exists(parent, dirname, &target);
	if (!found) {
		if (parent.type == EN_DIR && !strcmp(parent.name, dirname)) {
			target = parent;
			inode_t inode = get_inode(target.inode);
			if (inode.parent == target.inode) return -3; /* Root directory */
			memset(&parent, 0, sizeof(parent));
			parent.type = EN_DIR;
			parent.inode = inode.parent;
		}
		else {
			return -4; /* Directory missing */
		}
	}

	if (target.type != EN_DIR) return -5; /* Target is not a directory */
	if (!strcmp(target.name, ".") || !strcmp(target.name, "..")) return -2; /* Reject dot entries */

	dir_iter_t it;
	if (dir_open(target.inode, &it) != 0) return -7; /* Unable to read directory */

	dirent_t entry;
	while (dir_next(&it, &entry) == 1) {
		if (!strcmp(entry.name, ".") || !strcmp(entry.name, "..")) continue;
		dir_close(&it);
		return -6; /* Directory not empty */
	}
	dir_close(&it);

	if (dir_remove_entry(parent, target) != 0) return -7; /* Could not update parent */

	inode_release(target.inode);
	return 0;
}

/* Delete a file. In the future it'll remove links until non remain and then delete it */
int32_t unlink(const char* path, dirent_t cwd) {
	if (path == NULL) return -1;

	dirent_t parent;
	uint8_t status = resolve_dir(path, cwd, &parent);
	if (status != 0) return -1; /* Invalid path */

	char filename[FILENAME_LEN];
	status = path_to_name(path, filename);
	if (status != 2) return -2; /* Invalid filename */

	dirent_t target;
	if (!dir_child_exists(parent, filename, &target)) {
		if (parent.type == EN_DIR && !strcmp(parent.name, filename)) return -4; /* Directory targeted */
		return -3; /* Target missing */
	}
	if (target.type != EN_FILE) return -4; /* Target not a file */

	for (uint8_t i = 0; i < OFT_MAX; ++i) {
		if (boot_fsd->oft[i].state == OPEN && boot_fsd->oft[i].in_index == target.inode) return -5; /* File is open */
	}

	if (dir_remove_entry(parent, target) != 0) return -6; /* Could not update parent */

	inode_release(target.inode);
	return 0;
}

/* Kernel helper function to create and quickly write a string to a file */
uint8_t create_write(const char* filename, const char* str, dirent_t cwd) {
	if (filename == NULL || str == NULL) return 1;
	if (create(filename, cwd) != 0) return 1;
	FILE f;
	if (open(filename, &f, cwd) != 0) return 1;
	uint32_t len = strlen(str);
	uint32_t written = write(&f, (byte*)str, len);
	close(&f);
	if (written != len) return 1;
	return 0;
}
