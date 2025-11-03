#include <fs/fs.h>

/* 'fat_get' returns the next FAT table block index     *
 * after the current index. Or a status code if the     *
 * block is reserved or free.                           */
int16_t fat_get(int16_t idx) {
	if (idx > boot_fsd->device->num_blocks - 1 || idx < 0) return FAT_BAD;
	byte* base = boot_fsd->device->ramdisk + (boot_fsd->super.fat_head * boot_fsd->device->block_size);
	int16_t* fat = (int16_t*)base;
	return fat[idx];
}

/* 'fat_set' sets a value to the specified FAT table   *
 * block index. On a success, it returns the value     *
 * written to the table entry.                         */
int16_t fat_set(int16_t idx, int16_t val) {
	if (idx > boot_fsd->device->num_blocks - 1 || idx < 0
		|| val > boot_fsd->device->num_blocks - 1) return FAT_BAD;
	byte* base = boot_fsd->device->ramdisk + (boot_fsd->super.fat_head * boot_fsd->device->block_size);
	int16_t* fat = (int16_t*)base;
	fat[idx] = val;
	return val;
}