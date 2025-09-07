#include <importer.h>
#include <barelib.h>
#include <malloc.h>
#include <fs.h>

void importer(void) {
    return;
}

uint16 bytes_to_u16(const byte* ptr) { /* Unsafe test function */
	uint16 value = 0;
	for (int i = 0; i < 16; ++i) {
        value |= ((uint16)ptr[i]) << (8 * i);
    }
    return value;
}

void IMPORT_TEST(void) {
    const byte BYTES_NEEDED = 372;
	byte* ptr = malloc(BYTES_NEEDED);
	char name_buff[17];
	for(int i = 0; i < 16; ++i) {
		name_buff[i] = *ptr;
		++ptr;
	}
	name_buff[16] = '\0';
	uint16 size = bytes_to_u16(ptr);
	ptr += 16;
	create(name_buff);
	uint32 fd = open(name_buff);
	write(fd, (char*)ptr, size);
	close(fd);
	kprintf("Imported file: %s\n", name_buff);
}