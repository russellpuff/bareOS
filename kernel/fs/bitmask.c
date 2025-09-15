#include <fs/fs.h>

static inline byte* bm_base(void) { return fsd->device.ramdisk + ((uint32_t)BM_BIT * fsd->device.block_size); }

/* Sets the block at index 'x' as used in the bitmask block. */
void bm_set(uint32_t x) {
	if (fsd == NULL) return;
	byte* b = bm_base();
	b[x / 8] |= 0x1 << (x % 8);
}

/* Sets the block at index 'x' as free in the bitmask block. */
void bm_clear(uint32_t x) {
	if (fsd == NULL) return;
	byte* b = bm_base();
	b[x / 8] &= ~(0x1 << (x % 8));
}

/* Returns the current value of the block at index 'x' in the block device. 0 = free, 1 = used. */
byte bm_get(uint32_t x) {
	if (fsd == NULL) return -1;
	byte* b = bm_base();
	return (b[x / 8] >> (x % 8)) & 0x1;
}

/* Finds the next free block in the block device. */
int32_t bm_findfree(void) {
	byte* b = bm_base();
	for (int i = 0; i < fsd->device.block_size; ++i) {
		if (b[i] != 0xFF) {
			for (int j = 0; j < 8; ++j) {
				if (((b[i] >> j) & 0x1) == 0x0) {
					return (i * 8) + j;
				}
			}
		}
	}
	return -1;
}