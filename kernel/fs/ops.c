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
uint32_t read(FILE* f, byte* buff, uint32_t len) {
	if (f->fd >= OFT_MAX) return 0;
	filetable_t* entry = &boot_fsd->oft[f->fd];
	if (entry->state != OPEN || len == 0) return 0;
	//if (entry->curr_index >= entry->inode.size) return 0;
	uint32_t bytes_read = iread(*entry->inode, buff, 0, len);
	//entry->curr_index += bytes_read;
	return bytes_read;
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
uint32_t write(FILE* f, byte* buff, uint32_t len) {
	if (f->fd >= OFT_MAX) return 0;
	filetable_t* entry = &boot_fsd->oft[f->fd];
	if (entry->state != OPEN || len == 0) return 0;
	uint32_t written = iwrite(entry->inode, buff, 0, len);
	//entry->curr_index += written;
	entry->in_dirty = true;
	return written;
}

/* Ensures that a block at a logical index actually exists, and if not *
 * tries to expand the inode's blocks to cover the requested index     */
static int16_t ensure_block(inode_t* inode, uint32_t logical_idx) {
	if (inode->head < 0) {
		int16_t blk = allocate_block();
		if (blk == FAT_BAD) return FAT_BAD;
		inode->head = blk;
	}

	int16_t block = inode->head;
	for (uint32_t idx = 0; idx < logical_idx; ++idx) {
		int16_t next = fat_get(block);
		switch (next) {
			case FAT_BAD: return FAT_BAD;
			case FAT_FREE:
				// todo: read error
				return FAT_BAD;
			case FAT_END:
				int16_t new_block = allocate_block();
				if (new_block == FAT_BAD) return FAT_BAD;
				fat_set(block, new_block);
				next = new_block;
		}
		block = next;
	}
	return block;
}

/* FAT table walker that finds the block that corresponds to an 'index'  *
 * if you consider an inode's blocks a continuous array.                 *
 * So if an inode has blocks 5, 7, 9, 21, and you're looking for the 4th *
 * block, passing in index '3' returns 21.                               */
int16_t index_to_block(inode_t inode, uint32_t logical_idx) {
	if (inode.head < 0) return FAT_BAD;
	int16_t block = inode.head;
	for (uint32_t idx = 0; idx < logical_idx; ++idx) {
		int16_t next = fat_get(block);
		switch (next) {
			case FAT_BAD: return FAT_BAD;
			case FAT_END: return FAT_END;
			case FAT_FREE: 
				// todo: read error
				return FAT_BAD;
		}
		block = next;
	}
	return block;
}

/* Function reads from the raw blocks pointed to by an inode */
uint32_t iread(inode_t inode, byte* buff, uint32_t offset, uint32_t len) {
	if (inode.head < 0 || len == 0) return 0;
	if (offset >= inode.size) return 0;

	uint32_t block_size = boot_fsd->device->block_size;
	uint32_t available = inode.size - offset;
	if (len > available) len = available;

	uint32_t block_idx = offset / block_size;
	int16_t block = index_to_block(inode, block_idx);
	if (block == FAT_BAD) {
		// todo: read error
		return 0;
	}
	if (block == FAT_END) return 0;

	uint32_t block_offset = offset % block_size;
	uint32_t remaining = len;
	uint32_t copied = 0;

	byte* out = buff;
	while (remaining > 0 && block >= 0) {
		uint32_t chunk_sz = block_size - block_offset;
		if (chunk_sz > remaining) chunk_sz = remaining;
		read_bdev(block, block_offset, out, chunk_sz);
		out += chunk_sz;
		copied += chunk_sz;
		remaining -= chunk_sz;
		block_offset = 0;
		if (remaining == 0) break;
		int16_t next = fat_get(block);
		if (next == FAT_BAD) {
			// TODO: read error
			break;
		}
		if (next == FAT_END) break;
		block = next;
	}

	return copied;
}

/* 'iwrite' takes an inode and writes to its blocks directly                      *
 * Returns the number of bytes written (even if truncated due to lack of storage) */
uint32_t iwrite(inode_t* inode, byte* buff, uint32_t offset, uint32_t len) {
	if (len == 0) return 0;

	uint32_t block_size = boot_fsd->device->block_size;
	uint32_t block_idx = offset / block_size;
	uint32_t block_offset = offset % block_size;
	uint32_t remaining = len;
	uint32_t written = 0;

	int16_t block = ensure_block(inode, block_idx);
	if (block == FAT_BAD) {
		// todo: write error
		return 0;
	}

	byte* src = buff;
	while (remaining > 0 && block >= 0) {
		uint32_t chunk = block_size - block_offset;
		if (chunk > remaining) chunk = remaining;

		write_bdev(block, block_offset, src, chunk);

		written += chunk;
		remaining -= chunk;
		src += chunk;
		block_offset = 0;

		if (remaining == 0) break;

		int16_t next = fat_get(block);
		if (next == FAT_BAD || next == FAT_FREE) {
			// todo: write error
			// until this todo is complete, this corrupts the file
			// doesn't matter, fs is already corrupt if this hits
			return 0;
		}
		if (next == FAT_END) {
			int16_t new_block = allocate_block();
			if (new_block == FAT_BAD) {
				// todo: write error
				break;
			}
			fat_set(block, new_block);
			fat_set(new_block, FAT_END);
			block = new_block;
		} else {
			block = next;
		}
	}

	uint32_t new_size = offset + written;
	if (new_size > inode->size) inode->size = new_size;

	return (int32_t)written;
}

/* Finds the next non-free entry in a directory and puts it in 'out' *
 * Returns 1 if found, 0 if not found, or -1 on an error             */
int16_t dir_next(dir_iter_t* it, dirent_t* out) {
	if (it == NULL) return -1;
	while (it->offset + sizeof(dirent_t) <= it->sz) {
		if (iread(it->inode, (byte*)out, it->offset, sizeof(dirent_t)) != sizeof(dirent_t)) return -1;
		it->offset += sizeof(dirent_t);
		if (out->type == EN_FREE) continue;
		return 1;
	}
	return 0;
}

/* Function takes a parent dirent_t and writes a new entry to the first *
 * free slot it can find in that directory's inode blocks               *
 * Returns 0 on success, 1 if bad call, 2 if write error                */
uint8_t dir_write_entry(dirent_t parent, dirent_t entry) {
	if (parent.type != EN_DIR) return 1;
	inode_t p_ino = get_inode(parent.inode);
	uint32_t entries = p_ino.size / sizeof(dirent_t); /* Includes entries that were freed but not defragmented, will reuse free */
	uint32_t free_offset = (uint32_t)-1; /* Sentinel value by default */
	dirent_t child;
	/* Scan directory for free child and get its offset */
	for (uint32_t i = 0; i < entries; ++i) {
		if (iread(p_ino, (byte*)&child, i * sizeof(dirent_t), sizeof(dirent_t)) != sizeof(dirent_t)) continue;
		if (child.type == EN_FREE) { free_offset = i * sizeof(dirent_t); break; } /* Found an open slot in the allocated blocks */
	}
	free_offset = free_offset == (uint32_t)-1 ? p_ino.size : free_offset;
	if (iwrite(&p_ino, (byte*)&entry, free_offset, sizeof(dirent_t)) != sizeof(dirent_t)) return 2;
	write_inode(p_ino, parent.inode);
	return 0;
}
