#include <lib/bareio.h>
#include <system/version.h>
#include <system/thread.h>
#include <system/interrupts.h>
#include <system/queue.h>
#include <system/panic.h>
#include <system/memlayout.h>
#include <mm/malloc.h>
#include <mm/vm.h>
#include <device/tty.h>
#include <device/uart.h>
#include <device/rtc.h>
#include <fs/fs.h>
#include <fs/importer.h>
#include <util/string.h>
#include <dev/ecall.h>
#include <dev/time.h>

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
	init_rtc();
	init_pages();
	init_interrupts();
}

/* This function displays the welcome screen when the system and shell boot. */
static void display_welcome(void) {
	datetime now = rtc_read_datetime();
	char time[TIME_BUFF_SZ];
	dt_to_string(now, time, TIME_BUFF_SZ);

	kprintf("Welcome to bareOS alpha%d-%d.%d.%d (qemu-system-riscv64)\n\n", VERSION_ALPHA, VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
	kprintf("  Kernel information as of %s\n\n", time);
	kprintf("  Kernel start: %x\n  Kernel size: %d\n  Globals start: %x\n  Heap/Stack start: %x\n  Free Memory Available: %d\n\n",
		(uint64_t)&text_start,
		(uint64_t)(&data_start - &text_start),
		(uint64_t)&data_start,
		(uint64_t)&mem_start,
		(uint64_t)(&mem_end - &mem_start));
	
	/* Query the importer log to determine whether the importer finished cleanly. */
	const char sentinel[] = "Importer finished with no errors.";
	const char* log_path = "/etc/importer.log";
	FILE f = { 0 };
	f.fd = (FD)-1;
	bool importer_ok = false;
	bool log_available = open(log_path, &f, boot_fsd->super.root_dirent) == 0;
	if (log_available) {
		const uint16_t BUFFER_SIZE = 1024;
		char buffer[BUFFER_SIZE];
		memset(buffer, '\0', BUFFER_SIZE);
		uint32_t read_bytes = read(&f, (byte*)buffer, BUFFER_SIZE - 1);
		if (read_bytes >= BUFFER_SIZE) buffer[BUFFER_SIZE - 1] = '\0';
		close(&f);
		uint32_t sentinel_len = strlen(sentinel);
		for (uint32_t idx = 0; idx + sentinel_len <= read_bytes; ++idx) {
			if (memcmp(buffer + idx, sentinel, sentinel_len) == 0) {
				importer_ok = true;
				break;
			}
		}
	}
	const char* result = importer_ok
		? "The importer finished successfully."
		: "The importer ran into an error.";
	const char* detail = log_available
		? "Check /etc/importer.log for more details."
		: "Importer log unavailable.";
	kprintf("%s %s\n\n", result, detail);
}

static void sys_idle() { while (1); }

static void root_thread(void) {
	change_localtime("est");
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

	dirent_t bin;
	if (!dir_child_exists(boot_fsd->super.root_dirent, "bin", &bin))
		panic("Fatal: /bin missing, cannot start shell.\n");

	FILE f;
	f.fd = (FD)-1;
	open("shell.elf", &f, bin);
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

