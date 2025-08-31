#include <barelib.h>
#include <bareio.h>
#include <interrupts.h>
#include <thread.h>
#include <queue.h>
#include <malloc.h>
#include <tty.h>
#include <fs.h>

#define IMPORT_BASE 0x84000000

void display_kernel_info(void) {
  kprintf("Kernel start: %x\n--Kernel size: %d\nGlobals start: %x\nHeap/Stack start: %x\n--Free Memory Available: %d\nEnd of memory: %x\n",
    (unsigned long)&text_start,
    (unsigned long)(&data_start - &text_start),
    (unsigned long)&data_start,
    (unsigned long)&mem_start,
    (unsigned long)(&mem_end - &mem_start),
    (unsigned long)(&mem_end));
}

void test_ramdisk_probe(void) {
	char *ptr = (char*)IMPORT_BASE;
	kprintf("Imported bytes: %s\n", ptr);
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
	mk_ramdisk(MDEV_BLOCK_SIZE, MDEV_NUM_BLOCKS);
	mkfs();
	mount_fs();
	test_ramdisk_probe();
}
