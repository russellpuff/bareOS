#include <fs/fs.h>
#include <lib/string.h>

#define IN_PER_BLOCK (uint16_t)(boot_fsd->device->block_size / sizeof(inode_t))

static inline byte* get_block(uint16_t index) {
	return boot_fsd->device->ramdisk + (index * boot_fsd->device->block_size);
}

/* 'in_find_free' walks the inode table through its secret FAT path known to *
 * the fsd super. If it finds a free entry, returns the index. Otherwise it  *
 * will try to allocate a new block for the table and then returns an index  */
uint16_t in_find_free(void) {
	for (byte blk_idx = 0; blk_idx < boot_fsd->super.intable_numblks; ++blk_idx) {
		inode_t* block = (inode_t*)get_block(boot_fsd->super.intable_blocks[blk_idx]);
		for (byte in = 0; in < IN_PER_BLOCK; ++in) {
			inode_t* inode = &block[in];
			if (inode->type == FREE) { /* A previously-freed or zeroed-out block should work here. */
				memset((byte*)inode, 0, sizeof(inode_t));
				inode->type = BUSY; /* Prevent theft. */
				return ((uint16_t)blk_idx * IN_PER_BLOCK) + in;
			}
		}
	}
	
	if (boot_fsd->super.intable_numblks < MAX_INTABLE_BLOCKS) {
		int32_t blk_idx = bm_findfree();
		if (blk_idx == -1) return IN_ERR; /* No free blocks anywhere in the bdev. */
		bm_set(blk_idx);
		fat_set(blk_idx, FAT_RSVD); /* Mark as reserved in the FAT table to obfuscate contents. */
		boot_fsd->super.intable_blocks[boot_fsd->super.intable_numblks++] = blk_idx;
		inode_t* block = (inode_t*)get_block(blk_idx);
		memset((byte*)block, 0, boot_fsd->device->block_size); /* Zero whole block in case of leftover junk. */
		block->type = BUSY; /* Prevent theft. */
		write_super();
		return ((uint16_t)boot_fsd->super.intable_numblks - 1) * IN_PER_BLOCK;
	}
	return IN_ERR; /* Hit max intable blocks. */
}

/* Writes an in-memory inode to the inode table at index. Currently assumes valid index. */
byte write_inode(inode_t inode, uint16_t index) {
	uint16_t tbl_idx = index / IN_PER_BLOCK;
	uint16_t offset = index % IN_PER_BLOCK;
	inode_t* block = (inode_t*)get_block(boot_fsd->super.intable_blocks[tbl_idx]);
	block[offset] = inode;
	return 0;
}

inode_t get_inode(uint16_t index) {
	uint16_t tbl_idx = index / IN_PER_BLOCK;
	uint16_t offset = index % IN_PER_BLOCK;
	inode_t* block = (inode_t*)get_block(boot_fsd->super.intable_blocks[tbl_idx]);
	return block[offset];
}
