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

uint16 bytes_to_u16(const byte* ptr) { /* Unsafe test function */
	uint16 value = 0;
	for (int i = 0; i < 16; ++i) {
        value |= ((uint16)ptr[i]) << (8 * i);
    }
    return value;
}

void IMPORT_TEST(void) {
	byte *ptr = (byte*)IMPORT_BASE;
	char name_buff[17];
	for(int i = 0; i < 16; ++i) {
		name_buff[i] = *ptr;
		++ptr;
	}
	name_buff[16] = '\0';
	uint16 size = bytes_to_u16(ptr);
	ptr += 16;
	kprintf("Imported file name: %s size: %d content: %s\n", name_buff, size, ptr);
	create(name_buff);
	uint32 fd = open(name_buff);
	write(fd, (char*)ptr, size);
	close(fd);
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
	IMPORT_TEST();
}
