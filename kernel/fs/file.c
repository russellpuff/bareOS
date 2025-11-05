#include <fs/fs.h>
#include <mm/malloc.h>
#include <lib/string.h>

/* Resolve the directory from the provided filepath, if invalid or the file  *
 * already exists at that path, return an error. Otherwise create a new      *
 * dirent for that directory for this file, create an inode, and assign the  *
 * new file a single block as its head block.                                */
int32_t create(char* path, dirent_t cwd) {
    if (path == NULL) return -1;
    uint32_t name_len = strlen(path);
    if (name_len == 0 || name_len >= FILENAME_LEN) return -1;

    dirent_t parent;
    uint8_t res = resolve_dir(path, &cwd, &parent);
    /* TODO: jfc normalize these return values */
    switch (res) {
        case 1: return -1;
        case 2: return -2;
        case 3: return -3;
        case 4: return -4;
    }

    inode_t dir_inode = get_inode(parent.inode);

    // below this is un-refactored code, you can ignore the "uses root_dirent" obvious problem

    uint32_t entries = dir_inode.size / sizeof(dirent_t);
    uint32_t free_offset = 0xFFFFFFFF;
    dirent_t entry;

    for (uint32_t idx = 0; idx < entries; ++idx) {
        if (iread(dir_inode, (byte*)&entry, idx * sizeof(dirent_t), sizeof(dirent_t)) != sizeof(dirent_t)) continue;
        if (entry.type == FREE) {
            if (free_offset == 0xFFFFFFFF) free_offset = idx * sizeof(dirent_t);
            continue;
        }
        if (!strcmp(entry.name, path)) return -1;
    }

    uint32_t target_offset = (free_offset == 0xFFFFFFFF) ? dir_inode.size : free_offset;

    uint16_t inode_index = in_find_free();
    if (inode_index == IN_ERR) return -1;

    inode_t node;
    memset(&node, 0, sizeof(node));
    node.type = FILE;
    node.parent = boot_fsd->super.root_dirent.inode;
    node.size = 0;
    node.modified = 0;

    int16_t head = allocate_block();
    if (head == FAT_BAD) {
        inode_t reset;
        memset(&reset, 0, sizeof(reset));
        reset.type = FREE;
        write_inode(reset, inode_index);
        return -1;
    }
    node.head = head;

    write_inode(node, inode_index);

    dirent_t new_entry;
    memset(&new_entry, 0, sizeof(new_entry));
    new_entry.inode = inode_index;
    new_entry.type = FILE;
    memcpy(new_entry.name, path, name_len);
    new_entry.name[name_len] = '\0';

    int32_t written = iwrite(&dir_inode, (byte*)&new_entry, target_offset, sizeof(new_entry));
    if (written != sizeof(new_entry)) {
        bm_clear(head);
        fat_set(head, FAT_FREE);
        inode_t reset;
        memset(&reset, 0, sizeof(reset));
        reset.type = FREE;
        write_inode(reset, inode_index);
        return -1;
    }

    write_inode(dir_inode, boot_fsd->super.root_dirent.inode);

    return 0;
}

/* Resolve the directory from the provided filepath, if invalid or the  *
 * file does not exist in the target directory, return an error.        *
 * otherwise find a free slot in the fsd's open file table and init for *
 * further operations on the file. Return an fd (index to oft)          */
int32_t open(char* path) {
    /* TEMP: No mechanism exists to resolve a path yet, until then,
        assume the target file is in the current (root) directory    */
    if (path == NULL) return -1;
    uint64_t name_len = strlen(path);
    if (name_len == 0 || name_len >= FILENAME_LEN) return -1;
    /* TODO: replace with resolved parent dir */
    
    inode_t dir_inode = get_inode(boot_fsd->super.root_dirent.inode);
    uint32_t entries = dir_inode.size / sizeof(dirent_t);
    dirent_t entry;
    bool found = false;

    for (uint32_t idx = 0; idx < entries; ++idx) {
        if (iread(dir_inode, (byte*)&entry, idx * sizeof(dirent_t), sizeof(dirent_t)) != sizeof(dirent_t)) continue;
        if (entry.type == FREE) continue;
        if (!strcmp(entry.name, path)) { found = true; break; }
    }

    if (!found || entry.type != FILE) return -1;

    int32_t fd = -1;
    for (uint32_t i = 0; i < OFT_MAX; ++i) {
        if (boot_fsd->oft[i].state == CLOSED) {
            fd = (int32_t)i;
            break;
        }
    }
    if (fd == -1) return -1;

    filetable_t* file = &boot_fsd->oft[fd];
    file->state = OPEN;
    file->mode = RDWR;
    file->inode = get_inode(entry.inode);
    file->in_index = entry.inode;
    file->in_dirty = false;
    file->curr_index = 0;

    return fd;
}

/* Takes an index into the oft and closes the file in the table. *
 * Finishes up by writing the inode back to the inode table and  *
 * any other cleanup work required.                              */
int32_t close(int32_t fd) {
    if (fd < 0 || fd >= OFT_MAX) return -1;
    filetable_t* file = &boot_fsd->oft[fd];
    if (file->state != OPEN) return -1;

    if (file->in_dirty) {
        write_inode(file->inode, file->in_index);
        file->in_dirty = false;
    }

    file->state = CLOSED;
    file->mode = RD_ONLY;
    file->curr_index = 0;
    file->in_index = IN_ERR;
    memset(&file->inode, 0, sizeof(inode_t));

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

dirent_t mk_dir(char* path, dirent_t cwd) {
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
	write_inode(ino, dir.inode); /* TODO: Check for invalid. */

	/* Make default subdirectories */
	dirent_t self_dot = get_dot_entry(dir.inode, ".");
	dirent_t parent_dot = get_dot_entry(parent, "..");
	write_bdev(ino.head, 0, &self_dot, sizeof(dirent_t));
	write_bdev(ino.head, sizeof(dirent_t), &parent_dot, sizeof(dirent_t));

	if (dir.inode != ino.parent) {
		/* Write dirent to parent if self isn't parent. */
		inode_t p_ino = get_inode(parent);
		byte* buff = malloc(sizeof(dir));
		memcpy(buff, &dir, sizeof(dir)); /* seems dumb, maybe change one day? */
		iwrite(&p_ino, buff, p_ino.size, sizeof(dir)); /* TODO: if this returns a value not equal to sizeof(dir), we're out of blocks. */
		write_inode(p_ino, parent);
		free(buff);
	}

	return dir;
}

byte dir_open(uint16_t inode_idx, dir_iter_t* it) {
    if (it == NULL) return 1;
    inode_t inode = get_inode(inode_idx);
    if (inode.type != DIR) return 1;
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
