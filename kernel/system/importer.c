#include <importer.h>
#include <barelib.h>
#include <bareio.h>
#include <malloc.h>
#include <fs.h>
#include <bareio.h>

/* Mildly unsafe helper that reads the raw bytes from header to a uint16. */
uint16 bytes_to_u16(const byte* ptr) { 
	uint16 value = 0;
	for (int i = 0; i < 16; ++i) {
        value |= ((uint16)ptr[i]) << (8 * i);
    }
    return value;
}

void* do_malloc_import(void) {
	uint64 MAX_FS_CAPACITY = INODE_BLOCKS * MDEV_BLOCK_SIZE * DIR_SIZE;
	uint64 HEAD_SIZE = 32;
	uint64 TOTAL_HEAD_SIZE = HEAD_SIZE * DIR_SIZE;
	uint64 MASTER_HEAD_SIZE = 2;
	uint64 IMPORT_BYTES_NEEDED = MAX_FS_CAPACITY + TOTAL_HEAD_SIZE + MASTER_HEAD_SIZE;
	return malloc(IMPORT_BYTES_NEEDED);
}

byte importer(byte* ptr) {
	byte num_files = *(ptr++);
	kprintf("Importer is trying to import %d files...\n", num_files & 0xFF);
	for(byte i = 0; i < num_files; ++i) {
		char name[FILENAME_LEN]; /* Read the file's name. */
		for(int j = 0; j < FILENAME_LEN; ++j) {
			name[j] = *ptr;
			++ptr;
			if(name[j] == '\0') { ptr += (FILENAME_LEN - j - 1); break; }
		}
		/* Write to fd */
		uint16 size = bytes_to_u16(ptr);
		ptr += 16;
		if(create(name) == -1) return -1;
		uint32 fd = open(name);
		write(fd, (char*)ptr, size);
		close(fd);
		kprintf("Importer wrote %s (%u bytes).\n", name, size);
		ptr += size;
	}
	kprintf("Importer finished with no errors.\n");
    return 0;
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
	kprintf("Number of imported files: %d\n", num_files & 0xFF);
	kprintf("Imported file: %s\n", name_buff);
}
