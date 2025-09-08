#include <importer.h>
#include <barelib.h>
#include <bareio.h>
#include <malloc.h>
#include <fs.h>

#include <bareio.h>

void* do_malloc_import(void) {
	uint64 MAX_FS_CAPACITY = INODE_BLOCKS * MDEV_BLOCK_SIZE * DIR_SIZE;
	uint64 HEAD_SIZE = 32;
	uint64 TOTAL_HEAD_SIZE = HEAD_SIZE * DIR_SIZE;
	uint64 MASTER_HEAD_SIZE = 2;
	uint64 IMPORT_BYTES_NEEDED = MAX_FS_CAPACITY + TOTAL_HEAD_SIZE + MASTER_HEAD_SIZE;
	kprintf("bytes to malloc: %lu\n", IMPORT_BYTES_NEEDED);
	return malloc(IMPORT_BYTES_NEEDED);
}

void importer(void) {
    return;
}

uint16 bytes_to_u16(const byte* ptr) { /* Unsafe test function */
	uint16 value = 0;
	for (int i = 0; i < 16; ++i) {
        value |= ((uint16)ptr[i]) << (8 * i);
    }
    return value;
}

void IMPORT_TEST(byte* ptr) {
	byte num_files = *(ptr++);
	char name_buff[17];
	for(int i = 0; i < 16; ++i) {
		name_buff[i] = *ptr;
		++ptr;
	}
	name_buff[16] = '\0';
	uint16 size = bytes_to_u16(ptr);
	ptr += 16;
	create(name_buff);
	uint32 fd = open(name_buff);
	write(fd, (char*)ptr, size);
	close(fd);
	kprintf("Number of imported files: %d", num_files & 0xFF);
	kprintf("Imported file: %s\n", name_buff);
}
