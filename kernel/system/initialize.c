#include <barelib.h>
#include <bareio.h>
#include <interrupts.h>
#include <thread.h>
#include <queue.h>
#include <queue.h>
#include <malloc.h>
#include <tty.h>
#include <fs.h>
#include <importer.h>
#include <version.h>
#include <string.h>

void display_welcome(void) {
	kprintf("Welcome to bareOS alpha%d-%d.%d.%d (qemu-system-riscv64)\n\n", VERSION_ALPHA, VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
	kprintf("  Kernel information as of Who Knows When\n\n");
	kprintf("  Kernel start: %x\n  Kernel size: %d\n  Globals start: %x\n  Heap/Stack start: %x\n  Free Memory Available: %d\n\n",
		(unsigned long)&text_start,
		(unsigned long)(&data_start - &text_start),
		(unsigned long)&data_start,
		(unsigned long)&mem_start,
		(unsigned long)(&mem_end - &mem_start));
	
	/* Janky way of detecting importer status on boot. */
	char sentinel[] = "Importer finished with no errors.";
	int16_t fd = open("importer.log");
	uint16_t BUFFER_SIZE = 1024;
	char buffer[BUFFER_SIZE];
	memset(buffer, '\0', BUFFER_SIZE);
	read(fd, buffer, BUFFER_SIZE);
	close(fd);
	byte ok = 0;
	for (char *p = buffer; *p; ++p) { 
		const char *s = p, *t = sentinel;
		while (*t && *s == *t) { ++s; ++t; }
		if (*t == '\0') { ok = 1; break; }  // matched full sentinel
	}
	const char *result = ok 
		? "The importer finished successfully." 
		: "The importer ran into an error.";
	kprintf("%s Check importer.log for more details.\n\n", result);
}

/*
 *  This function calls any initialization functions to set up
 *  devices and other systems.
 */
void initialize(void) {
	init_tty();
	init_uart();
	init_interrupts();
	init_threads();
	init_queues();
	init_heap();
	byte* imp = malloc_loaded_range(); /* QEMU loader injects at top of freelist. So we steal it asap. */
	mk_ramdisk(MDEV_BLOCK_SIZE, MDEV_NUM_BLOCKS);
	mkfs();
	mount_fs();
	if(generic_importer(imp) != 0) kprintf("The importer encountered an error and had to stop.\n");
	free(imp); 
}
