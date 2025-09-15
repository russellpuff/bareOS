#include <fs/fs.h>
#include <fs/importer.h>
#include <lib/barelib.h>
#include <lib/bareio.h>
#include <lib/string.h>
#include <mm/malloc.h>

/* Mildly unsafe helper that reads the raw bytes from header to a uint16. */
uint16_t bytes_to_u16(const byte* ptr) { 
	uint16_t value = 0;
	for (int i = 0; i < 16; ++i) {
        value |= ((uint16_t)ptr[i]) << (8 * i);
    }
    return value;
}

/* Important helper functon that mallocs enough bytes to handle maxing out the  *
 * ramdisk instantly. As long as this runs as the system's first call to malloc *
 * the data injected by the generic loader is safe.                             */
void* malloc_loaded_range(void) {
	uint64_t MAX_FS_CAPACITY = INODE_BLOCKS * BDEV_BLOCK_SIZE * DIR_SIZE;
	uint64_t HEAD_SIZE = 32;
	uint64_t TOTAL_HEAD_SIZE = HEAD_SIZE * DIR_SIZE;
	uint64_t MASTER_HEAD_SIZE = 2;
	uint64_t IMPORT_BYTES_NEEDED = MAX_FS_CAPACITY + TOTAL_HEAD_SIZE + MASTER_HEAD_SIZE;
	return malloc(IMPORT_BYTES_NEEDED);
}

/* Basic helper function to advance a pointer to the end of a c string. */
byte* run_to_nc(byte* ptr) {
	while(*ptr != '\0') ++ptr;
	return ptr;
}

/* Generic importer, companion function to the QEMU generic loader. Reads the  *
 * data injected into the top of the heap by the loader and writes it to the   *
 * ramdisk.                                                                    */
byte generic_importer(byte* ptr) {
	byte status = 0;
	const uint16_t BUFFER_SIZE = 1024;
	char buffer[BUFFER_SIZE];
	byte* bptr = (byte*)buffer;
	memset(buffer, '\0', BUFFER_SIZE);
	byte num_files = *(ptr++);
	ksprintf(bptr, "Importer is trying to import %d files...\n", num_files & 0xFF);
	bptr = run_to_nc(bptr);
	for(byte i = 0; i < num_files; ++i) {
		char name[FILENAME_LEN]; /* Read the file's name. */
		for(int j = 0; j < FILENAME_LEN; ++j) {
			name[j] = *ptr;
			++ptr;
			if(name[j] == '\0') { ptr += (FILENAME_LEN - j - 1); break; }
		}
		/* Write to fd */
		uint16_t size = bytes_to_u16(ptr);
		ptr += 16;
		if(create(name) == -1) {
			status = -1;
			break;
		}
		uint32_t fd = open(name);
		write(fd, (char*)ptr, size);
		close(fd);
		ksprintf(bptr, "Importer wrote %s (%u bytes).\n", name, size);
		bptr = run_to_nc(bptr);
		ptr += size;
	}
	
	if(status == 0) { ksprintf(bptr, "Importer finished with no errors.\n"); } 
	else { ksprintf(bptr, "The importer had an error and had to stop: couldn't create one of the files.\n"); }
	bptr = run_to_nc(bptr);
	
	char n[] = "importer.log\0";
	create(n);
	uint32_t nfd = open(n);
	write(nfd, buffer, (bptr - (byte*)buffer) + 1);
	close(nfd);
    return 0;
}
