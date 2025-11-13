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
static void display_welcome(bool dailyMsg) {
	datetime now = rtc_read_datetime();
	char time[TIME_BUFF_SZ];
	dt_to_string(now, time, TIME_BUFF_SZ);

	dirent_t root = boot_fsd->super.root_dirent;

	const uint16_t BUFF_SZ = 1024;
	byte file_buff[BUFF_SZ];
	memset(file_buff, '\0', BUFF_SZ);

	ksprintf(file_buff,
		"Welcome to bareOS alpha%d-%d.%d.%d (qemu-system-riscv64)\n\n"
		"  Kernel information as of %s\n\n"
		"  Kernel start: %x\n  Kernel size: %d\n  Globals start: %x\n  Heap/Stack start: %x\n  Free Memory Available: %d\n\n",
		VERSION_ALPHA, VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH,
		time,
		(uint64_t)&text_start,
		(uint64_t)(&data_start - &text_start),
		(uint64_t)&data_start,
		(uint64_t)&mem_start,
		(uint64_t)(&mem_end - &mem_start)
	);
	
	if (dailyMsg) {
		byte* p = file_buff;
		while (*p++ != '\0');
		--p;

		/* Query the importer log to determine whether the importer finished cleanly. */
		const char sentinel[] = "Importer finished with no errors.";
		const char* log_path = "/etc/importer.log";
		FILE f = { 0 };
		f.fd = (FD)-1;
		bool importer_ok = false;
		bool log_available = open(log_path, &f, root) == 0;
		if (log_available) {
			char* buffer = (char*)malloc(f.inode.size + 1);
			buffer[f.inode.size] = '\0';
			read(&f, (byte*)buffer, f.inode.size);
			importer_ok = strstr(buffer, sentinel) != NULL; /* Janky way of detecting status */
			free(buffer);
			close(&f);
		}
		const char* result = importer_ok
			? "The importer finished successfully."
			: "The importer ran into an error.";
		const char* detail = log_available
			? "Check /etc/importer.log for more details."
			: "Importer log unavailable.";
		ksprintf(p, "%s %s\n\n", result, detail);
	}

	char* fname = "/etc/.welcome";
	if (create(fname, root) != 0) {
		unlink(fname, root);
		create(fname, root);
	}
	FILE f;
	open(fname, &f, root);
	write(&f, file_buff, strlen((const char*)file_buff));
	close(&f);
}

static void sys_idle() { while (1); }

static void root_thread(void) {
	change_localtime("est");
	display_welcome(true);
	uint32_t idle_tid = create_thread(&sys_idle, MODE_S);
	resume_thread(idle_tid);
	uint32_t reaper_tid = create_thread(&reaper, MODE_S);
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
	/* Spawning the shell will block execution of this thread until it finishes. 
	   Then we just restart it. No logout mechanism exists.                      */
	while (1) {
		ecall_spawn("shell", NULL);
		display_welcome(false);
	}
}

/*
 *  First c function called
 *  Used to initialize devices before starting steady state behavior
 */
void supervisor_start(void) {
	initialize();
	uint32_t root_tid = create_thread(&root_thread, MODE_S);
	context_load(&thread_table[root_tid], root_tid);
	while(1);
}

