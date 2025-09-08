#include <importer.h>
#include <barelib.h>
#include <bareio.h>
#include <malloc.h>
#include <fs.h>
#include <bareio.h>
#include <string.h>

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

/* Basic helper function to advance a pointer to the end of a c string. */
byte* run_to_nc(byte* ptr) {
	while(*ptr != '\0') ++ptr;
	return ptr;
}

byte importer(byte* ptr) {
	byte status = 0;
	const uint16 BUFFER_SIZE = 1024;
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
		uint16 size = bytes_to_u16(ptr);
		ptr += 16;
		if(create(name) == -1) {
			status = -1;
			break;
		}
		uint32 fd = open(name);
		write(fd, (char*)ptr, size);
		close(fd);
		ksprintf(bptr, "Importer wrote %s (%u bytes).\n", name, size);
		bptr = run_to_nc(bptr);
		ptr += size;
	}
	
	if(status == 0) { ksprintf(bptr, "Importer finished with no errors."); } 
	else { ksprintf(bptr, "The importer had an error and had to stop: couldn't create one of the files."); }
	bptr = run_to_nc(bptr);
	
	char n[] = "importer.log\0";
	uint32 nfd = create(n);
	write(nfd, buffer, (bptr - (byte*)buffer) + 1);
	close(nfd);
    return 0;
}
