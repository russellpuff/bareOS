#include <fs/fs.h>
#include <fs/importer.h>
#include <lib/bareio.h>
#include <mm/kmalloc.h>
#include <util/string.h>
#include <barelib.h>

/* Mildly unsafe helper that reads the raw bytes from header to a uint32. */
static uint32_t bytes_to_u32(const byte* ptr) {
	uint32_t value = 0;
	for (uint32_t i = 0; i < 4; ++i) {
		value |= ((uint32_t)ptr[i]) << (8 * i);
	}
	return value;
}

/* Important helper functon that mallocs enough bytes to handle maxing out the  *
 * ramdisk instantly. As long as this runs as the system's first call to kmalloc *
 * the data injected by the generic loader is safe.                             */
void* malloc_loaded_range(void) {
	const uint64_t HEAD_SZ = 4 + FILENAME_LEN;
	const uint64_t MAX_FILES = 100;
	const uint64_t MAX_SIZE = 1.5 * 1024 * 1024;
	const uint64_t IMPORT_BYTES_NEEDED = MAX_SIZE + (HEAD_SZ * MAX_FILES);
	return kmalloc(IMPORT_BYTES_NEEDED);
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
	uint8_t num_files = *(ptr++); /* Read num files from first byte */

	/* Can support up to 100 files worth a combined total of 1.5MiB. Create log buffer big enough. */
	enum { SIZE_DIGITS_MAX = 7, PER_LINE_OVERHEAD = 64, LOG_HEADER = 64, LOG_FOOTER = 128 };
	const uint32_t LOG_PER_FILE = PER_LINE_OVERHEAD + FILENAME_LEN + SIZE_DIGITS_MAX;
	uint32_t log_bytes = LOG_HEADER + (uint32_t)num_files * LOG_PER_FILE + LOG_FOOTER;

	byte* status_msg = kmalloc(log_bytes);
	memset(status_msg, 0, log_bytes);
	byte* status_ptr = status_msg;

	ksprintf(status_ptr, "Importer is trying to import %d files...\n", num_files & 0xFF);
	status_ptr = run_to_nc(status_ptr);

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
		FILE f;
		open(name, &f, cwd);
		write(&f, ptr, size);
		close(&f);
		ksprintf(status_ptr, "Importer wrote %s (%u bytes).\n", name, size);
		status_ptr = run_to_nc(status_ptr);
		ptr += size;
	}
	
	if(status == 0) { ksprintf(status_ptr, "Importer finished with no errors.\n"); } 
	else { ksprintf(status_ptr, "The importer had an error and had to stop: couldn't create one of the files.\n"); }
	status_ptr = run_to_nc(status_ptr);
	
	char n[] = "/etc/importer.log\0";
	create_write(n, (char*)status_msg, boot_fsd->super.root_dirent);
	free(status_msg);
	return 0;
}
