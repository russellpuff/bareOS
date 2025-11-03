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
uint32_t read(uint32_t fd, byte* buff, uint32_t len) {
    if (fd >= OFT_MAX) return 0;
    filetable_t* file = &boot_fsd->oft[fd];
    if (file->state != OPEN || len == 0) return 0;

    uint32_t file_size = file->inode.size;
    if (file->curr_index >= file_size) return 0;

    uint32_t offset = file->curr_index;
    uint32_t remaining = file_size - offset;
    if (len < remaining) remaining = len;

    uint32_t block_size = boot_fsd->device->block_size;
    uint32_t block_idx = offset / block_size;
    int16_t block = index_to_block(&file->inode, block_idx);
    if (block == FAT_BAD) {
        // todo: read error
        return 0;
    }
    if (block == FAT_END) return 0;

    uint32_t block_offset = offset % block_size;
    uint32_t bytes_read = 0;

    while (remaining > 0 && block >= 0) {
        uint32_t chunk = block_size - block_offset;
        if (chunk > remaining) chunk = remaining;
        read_bdev(block, block_offset, buff, chunk);
        bytes_read += chunk;
        remaining -= chunk;
        buff += chunk;
        block_offset = 0;

        if (remaining == 0) break;

        int16_t next = fat_get(block);
        if (next == FAT_BAD) {
            // todo: read error
            return 0;
        }
        if (next == FAT_END) {
            block = next;
            break;
        }
        block = next;
    }

    file->curr_index += bytes_read;
    if (file->curr_index >= file->inode.size) {
        file->curr_block = 0xFFFF;
    } else {
        uint32_t new_block_idx = file->curr_index / block_size;
        int16_t new_block = index_to_block(&file->inode, new_block_idx);
        file->curr_block = (new_block < 0) ? 0xFFFF : (uint16_t)new_block;
    }

    return (int32_t)bytes_read;
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
uint32_t write(uint32_t fd, byte* buff, uint32_t len) {
    if (fd >= OFT_MAX) return 0;
    filetable_t* file = &boot_fsd->oft[fd];
    if (file->state != OPEN || len == 0) return 0;

    uint32_t written = iwrite(&file->inode, buff, file->curr_index, len);

    file->curr_index += written;
    file->in_dirty = true;

    uint32_t block_size = boot_fsd->device->block_size;
    if (file->curr_index >= file->inode.size) {
        file->curr_block = 0xFFFF;
    }
    else {
        uint32_t block_idx = file->curr_index / block_size;
        int16_t block = index_to_block(&file->inode, block_idx);
        file->curr_block = (block < 0) ? 0xFFFF : (uint16_t)block;
    }

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

/* Takes a file descriptor index into the 'oft' and returns that file's size for reading. */
uint32_t get_filesize(uint32_t fd) {
	return boot_fsd->oft[fd].inode.size;
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
int16_t index_to_block(inode_t* inode, uint32_t logical_idx) {
    if (inode->head < 0) return FAT_BAD;
    int16_t block = inode->head;
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
uint32_t iread(inode_t* inode, byte* buff, uint32_t offset, uint32_t len) {
    if (inode->head < 0 || len == 0) return 0;
    if (offset >= inode->size) return 0;

    uint32_t block_size = boot_fsd->device->block_size;
    uint32_t available = inode->size - offset;
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