#include <fs/fs.h>
#include <fs/importer.h>
#include <lib/barelib.h>
#include <lib/bareio.h>
#include <lib/string.h>
#include <mm/malloc.h>

/* Mildly unsafe helper that reads the raw bytes from header to a uint32. */
uint32_t bytes_to_u32(const byte* ptr) {
	uint32_t value = 0;
	for (uint32_t i = 0; i < 4; ++i) {
		value |= ((uint32_t)ptr[i]) << (8 * i);
	}
	return value;
}

/* Important helper functon that mallocs enough bytes to handle maxing out the  *
 * ramdisk instantly. As long as this runs as the system's first call to malloc *
 * the data injected by the generic loader is safe.                             */
void* malloc_loaded_range(void) {
	const uint64_t HEAD_SZ = 4 + FILENAME_LEN;
	const uint64_t MAX_FILES = 100;
	const uint64_t MAX_SIZE = 1.5 * 1024 * 1024;
	const uint64_t IMPORT_BYTES_NEEDED = MAX_SIZE + (HEAD_SZ * MAX_FILES);
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
	byte buffer[BUFFER_SIZE];
	byte* bptr = (byte*)buffer;
	memset(buffer, '\0', BUFFER_SIZE);
	byte num_files = *(ptr++);
	ksprintf(bptr, "Importer is trying to import %d files...\n", num_files & 0xFF);
	bptr = run_to_nc(bptr);
	for(byte i = 0; i < num_files; ++i) {
		char name[FILENAME_LEN]; /* Read the file's name. */
		for(byte j = 0; j < FILENAME_LEN; ++j) {
			name[j] = *ptr;
			++ptr;
			if(name[j] == '\0') { ptr += (FILENAME_LEN - j - 1); break; }
		}
		/* Write to fd */
		uint32_t size = bytes_to_u32(ptr);
		ptr += 4;
		if(create(name, boot_fsd->super.root_dirent) != 0) {
			status = -1;
			break;
		}
		FILE f = { 0 };
		open(name, &f, boot_fsd->super.root_dirent);
		write(&f, ptr, size);
		close(&f);
		ksprintf(bptr, "Importer wrote %s (%u bytes).\n", name, size);
		bptr = run_to_nc(bptr);
		ptr += size;
	}
	
	if(status == 0) { ksprintf(bptr, "Importer finished with no errors.\n"); } 
	else { ksprintf(bptr, "The importer had an error and had to stop: couldn't create one of the files.\n"); }
	bptr = run_to_nc(bptr);
	
	char n[] = "importer.log\0";
	create(n, boot_fsd->super.root_dirent);
	FILE f2 = { 0 };
	open(n, &f2, boot_fsd->super.root_dirent);
	write(&f2, buffer, bptr - buffer + 1);
	close(&f2);
	return 0;
}
