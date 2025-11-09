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
uint8_t generic_importer(byte* ptr) {
	uint8_t status = 0;
	const uint16_t BUFFER_SIZE = 1024;
	byte buffer[BUFFER_SIZE];
	byte* bptr = (byte*)buffer;
	memset(buffer, '\0', BUFFER_SIZE);
	uint8_t num_files = *(ptr++);
	ksprintf(bptr, "Importer is trying to import %d files...\n", num_files & 0xFF);
	bptr = run_to_nc(bptr);

	/* Set up working directories to determine where files go */
	dirent_t bin, home;
	if (!dir_child_exists(boot_fsd->super.root_dirent, "bin", &bin) ||
		!dir_child_exists(boot_fsd->super.root_dirent, "home", &home)) {
		status = 1;
		num_files = 0;
	}

	for(uint8_t i = 0; i < num_files; ++i) {
		char name[FILENAME_LEN]; /* Read the file's name. */
		for(uint8_t j = 0; j < FILENAME_LEN; ++j) {
			name[j] = *ptr;
			++ptr;
			if(name[j] == '\0') { ptr += (FILENAME_LEN - j - 1); break; }
		}

		/* Set cwd to /bin if it's an elf or /home otherwise */
		uint64_t sz = strlen(name);
		dirent_t cwd = sz >= 4 && memcmp(name + sz - 4, ".elf", 4) == 0 ? bin : home;

		/* Write to dir */
		uint32_t size = bytes_to_u32(ptr);
		ptr += 4;
		if(create(name, cwd) != 0) {
			status = -1;
			break;
		}
		FILE f = { 0 };
		open(name, &f, cwd);
		write(&f, ptr, size);
		close(&f);
		ksprintf(bptr, "Importer wrote %s (%u bytes).\n", name, size);
		bptr = run_to_nc(bptr);
		ptr += size;
	}
	
	if(status == 0) { ksprintf(bptr, "Importer finished with no errors.\n"); } 
	else { ksprintf(bptr, "The importer had an error and had to stop: couldn't create one of the files.\n"); }
	bptr = run_to_nc(bptr);
	
	char n[] = "/etc/importer.log\0";
	create(n, boot_fsd->super.root_dirent);
	FILE f2 = { 0 };
	open(n, &f2, boot_fsd->super.root_dirent);
	write(&f2, buffer, bptr - buffer + 1);
	close(&f2);
	return 0;
}
