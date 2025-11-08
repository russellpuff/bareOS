#include <lib/barelib.h>
#include <lib/bareio.h>
#include <lib/string.h>
#include <lib/ecall.h>
#include <system/version.h>
#include <system/thread.h>
#include <system/interrupts.h>
#include <system/queue.h>
#include <system/panic.h>
#include <mm/malloc.h>
#include <mm/vm.h>
#include <device/tty.h>
#include <fs/fs.h>
#include <fs/importer.h>

/*
 *  This file contains the C code entry point executed by the kernel.
 *  It is called by the bootstrap sequence once the hardware is configured.
 *  (see bootstrap.s)
 */

/*
 *  This function calls any initialization functions to set up
 *  devices and other systems.
 */
static void initialize(void) {
	init_tty();
	init_uart();
	init_threads();
	init_queues();
	init_heap();
	byte* imp = malloc_loaded_range(); /* QEMU loader injects at top of freelist. So we steal it asap. */
	// temporary fs behavior:
	// create new ramdisk on boot (no persistence between boots)
	// mount the lone ramdisk and set it as the boot_fsd
	// no point in bothering with any more than that until we can persist the disk
	mkfs(BDEV_BLOCK_SIZE, BDEV_NUM_BLOCKS);
	mount_fs(&reg_drives->drive, "/");
	boot_fsd = reg_drives->drive.fsd;
	generic_importer(imp);
	free(imp); 
	init_pages();
	init_interrupts();
}

/* This function displays the welcome screen when the system and shell boot. */
static void display_welcome(void) {
	kprintf("Welcome to bareOS alpha%d-%d.%d.%d (qemu-system-riscv64)\n\n", VERSION_ALPHA, VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
	kprintf("  Kernel information as of Who Knows When\n\n");
	kprintf("  Kernel start: %x\n  Kernel size: %d\n  Globals start: %x\n  Heap/Stack start: %x\n  Free Memory Available: %d\n\n",
		(uint64_t)&text_start,
		(uint64_t)(&data_start - &text_start),
		(uint64_t)&data_start,
		(uint64_t)&mem_start,
		(uint64_t)(&mem_end - &mem_start));
	
	/* Janky way of detecting importer status on boot. */
	char sentinel[] = "Importer finished with no errors.";
	FILE f;
	open("importer.log", &f, boot_fsd->super.root_dirent);
	uint16_t BUFFER_SIZE = 1024;
	char buffer[BUFFER_SIZE];
	memset(buffer, '\0', BUFFER_SIZE);
	read(&f, (byte*)buffer, BUFFER_SIZE);
	close(&f);
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

static void sys_idle() { while (1); }

static void root_thread(void) {
	display_welcome();
	uint32_t idle_tid = create_thread(&sys_idle, "", 0, MODE_S);
	resume_thread(idle_tid);
	uint32_t reaper_tid = create_thread(&reaper, "", 0, MODE_S);
	resume_thread(reaper_tid);

	/* When true, echoes all keyboard input but strips nonprint 
	   When false, filters and discards unhandled nonprint without echo
	   Preferred false unless you need to test something. 
	 */
	RAW_GETLINE = false; 

	FILE f;
	f.fd = (FD)-1;
	open("shell.elf", &f, boot_fsd->super.root_dirent);
	if (f.fd == (FD)-1)
		panic("Fatal: no shell to run on boot.\n");
	close(&f);
	ecall_spawn("shell", NULL);
}

/*
 *  First c function called
 *  Used to initialize devices before starting steady state behavior
 */
void supervisor_start(void) {
	initialize();
	uint32_t root_tid = create_thread(&root_thread, "", 0, MODE_S);
	context_load(&thread_table[root_tid], root_tid);
	while(1);
}

