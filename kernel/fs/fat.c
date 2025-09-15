#include <fs/fs.h>

/* 'fat_get' returns the next FAT table block index     *
 * after the current index. Or a status code if the     *
 * block is reserved or free.                           */
int16_t fat_get(int16_t i) {
	if (i > fsd->device.num_blocks - 1 || i < 0) return FAT_BAD;
	byte* base = fsd->device.ramdisk + (fsd->super.fat_head * fsd->device.block_size);
	int16_t* fat = (int16_t*)base;
	return fat[i];
}

/* 'fat_set' sets a value to the specified FAT table   *
 * block index. On a success, it returns the value     *
 * written to the table entry.                         */
int16_t fat_set(int16_t i, int16_t v) {
	if (i > fsd->device.num_blocks - 1 || i < 0
		|| v > fsd->device.num_blocks - 1) return FAT_BAD;
	byte* base = fsd->device.ramdisk + (fsd->super.fat_head * fsd->device.block_size);
	int16_t* fat = (int16_t*)base;
	fat[i] = v;
	return v;
}