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
	memset(entry->inode, 0, sizeof(inode_t));

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
	if (strcmp(dir_name, parent.name)) return -3; /* Directory exists, resolve_dir resolved the preexisting target dir */

	out->type = EN_DIR;
	out->inode = in_find_free();
	uint32_t len = strlen(dir_name);
	memcpy(out->name, dir_name, len);
	out->name[len] = '\0';

	inode_t inode;
	/* Allocate inode block */
	inode.head = allocate_block();
	if (inode.head == FAT_BAD) {
		memset(&inode, 0, sizeof(inode_t));
		inode.type = EN_FREE;
		write_inode(inode, out->inode);
		return -4; /* Error creating entry */
	}

	/* Make default subdirectories */
	dirent_t self_dot = get_dot_entry(out->inode, ".");
	dirent_t parent_dot = get_dot_entry(parent.inode, "..");
	dir_write_entry(*out, self_dot);
	dir_write_entry(*out, parent_dot);

	/* Write dirent to parent if self isn't parent. */
	if (out->inode != parent.inode) dir_write_entry(parent, *out);

	write_inode(inode, out->inode);

	return 0;
}

byte dir_open(uint16_t inode_idx, dir_iter_t* it) {
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
