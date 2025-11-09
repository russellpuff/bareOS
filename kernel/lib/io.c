#include <lib/barelib.h>
#include <lib/printf.h>
#include <lib/io.h>
#include <lib/ecall.h>
#include <lib/string.h>

#define UPRINTF_BUFF_SIZE 1024

/* Userland override for putc, without access to the tty/uart,
   we can only stuff the result into a buffer and let the kernel
   do the real printing                                          */
byte* printf_putc(char c, byte mode, byte* ptr) {
	(void)mode;
	*ptr++ = c;
	return ptr;
}

/* Behaves similarly to std printf, see the implementation of printf_core to view 
   what options you have available for args                                       */
void printf(const char* format, ...) {
	char buffer[UPRINTF_BUFF_SIZE];
	memset(buffer, '\0', UPRINTF_BUFF_SIZE);
	va_list ap;
	va_start(ap, format);
	printf_core(MODE_BUFFER, (byte*)buffer, format, ap);
	va_end(ap);
	uart_dev_opts options;
	options.length = strlen(buffer);
	options.buffer = (byte*)buffer;
	ecall_write(UART_DEV_NUM, (byte*)&options);
}

/* Behaves similarly to std sprinf, see above */
void sprintf(byte* buffer, const char* format, ...) {
	va_list ap;
	va_start(ap, format);
	printf_core(MODE_BUFFER, buffer, format, ap);
	va_end(ap);
}

/* Gets a line of characters from the tty/uart and converts
   it into a C string when either enter is pressed or the 
   buffer is populated with 'length' characters             */
int32_t gets(char* buffer, uint32_t length) {
	if (buffer == NULL || length == 0) return 0;
	uart_dev_opts options;
	options.length = length;
	options.buffer = (byte*)buffer;
	return (int32_t)ecall_read(UART_DEV_NUM, (byte*)&options);
}

/* Creates a file at path */
int8_t fcreate(const char* path) {
	disk_dev_opts options;
	options.file = NULL;
	options.mode = FILE_CREATE;
	options.buff_in = (byte*)path;
	options.buff_out = NULL;
	options.length = strlen(path);
	return ecall_open(DISK_DEV_NUM, (byte*)&options);
}

/* Opens a file at path. Without malloc it requires you to pass in your own FILE */
int8_t fopen(const char* path, FILE* file) {
	disk_dev_opts options;
	file->fd = (FD)-1;
	options.file = file;
	options.mode = FILE_OPEN;
	options.buff_in = (byte*)path;
	options.buff_out = NULL;
	options.length = strlen(path);
	return ecall_open(DISK_DEV_NUM, (byte*)&options);
}

/* Closes an open FILE */
int8_t fclose(FILE* file) {
	disk_dev_opts options;
	options.file = file;
	options.mode = 0; /* forgot about close when writing, is it necessary? */
	options.buff_in = NULL;
	options.buff_out = NULL;
	options.length = 0;
	return ecall_close(DISK_DEV_NUM, (byte*)&options);
}

/* Reads 'len' bytes into 'buffer' starting at index 0 from a file, no seek supported */
uint32_t fread(FILE* file, byte* buffer, uint32_t len) {
	disk_dev_opts options;
	options.file = file;
	options.mode = FILE_READ;
	options.buff_in = NULL;
	options.buff_out = buffer;
	options.length = len;
	return ecall_read(DISK_DEV_NUM, (byte*)&options);
}

/* Writes 'len' bytes from 'buffer' starting at index 0 into a file, no seek supported */
uint32_t fwrite(FILE* file, byte* buffer, uint32_t len) {
	disk_dev_opts options;
	options.file = file;
	options.mode = FILE_WRITE;
	options.buff_in = buffer;
	options.buff_out = NULL;
	options.length = len;
	return ecall_write(DISK_DEV_NUM, (byte*)&options);
}

void fdelete() {
	
}

/* Creates a new directory at path */
int8_t mkdir(const char* path) {
	dirent_t placeholder; /* mkdir returns the dirent_t of the directory but nothing uses it */
	disk_dev_opts options;
	options.file = NULL;
	options.mode = DIR_CREATE;
	options.buff_in = (byte*)path;
	options.buff_out = (byte*)&placeholder;
	options.length = 0;
	return ecall_open(DISK_DEV_NUM, (byte*)&options);
}

//int8_t rmdir() {
//	// unimplemented
//}

/* Function requires an array of dirent_t to hold the response
   The 'count' field is the maximum number of children the array can hold */
int8_t rddir(const char* path, dirent_t* out, uint32_t count) {
	disk_dev_opts options;
	options.file = NULL;
	options.mode = DIR_READ;
	options.buff_in = (byte*)path;
	options.buff_out = (byte*)out;
	options.length = count;
	uint32_t result = ecall_read(DISK_DEV_NUM, (byte*)&options);
	return (int8_t)result;
}

/* Function returns a full directory_t of the target dir
   If chdir is true, changes the process cwd to the output directory 
   If a path with dot aliases (e.g. "../child/") is passed in, the 
   'path' field of the directory_t will be populated with its absolute
   path from the root, use this with chdir = false to expand paths     */
int8_t getdir(const char* path, directory_t* out, bool chdir) {
	disk_dev_opts options;
	options.file = NULL;
	options.mode = DIR_OPEN;
	options.buff_out = (byte*)out;

	options.length = strlen(path);
	byte buffer[MAX_PATH_LEN + 1 + sizeof(bool)];
	memcpy(buffer, &chdir, sizeof(bool));
	memcpy(buffer + sizeof(bool), path, options.length);
	buffer[options.length + sizeof(bool)] = '\0';
	options.buff_in = buffer;

	return ecall_open(DISK_DEV_NUM, (byte*)&options);
}
