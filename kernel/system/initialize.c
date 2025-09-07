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

void display_kernel_info(void) {
  kprintf("Kernel start: %x\n--Kernel size: %d\nGlobals start: %x\nHeap/Stack start: %x\n--Free Memory Available: %d\nEnd of memory: %x\n",
    (unsigned long)&text_start,
    (unsigned long)(&data_start - &text_start),
    (unsigned long)&data_start,
    (unsigned long)&mem_start,
    (unsigned long)(&mem_end - &mem_start),
    (unsigned long)(&mem_end));
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
	byte* imp = do_malloc_import(); /* QEMU loader injects at top of freelist. So we steal it asap. */
	mk_ramdisk(MDEV_BLOCK_SIZE, MDEV_NUM_BLOCKS);
	mkfs();
	mount_fs();
	IMPORT_TEST(imp);
	free(imp); 
}
