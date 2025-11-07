#include <lib/barelib.h>
#include <lib/printf.h>
#include <lib/io.h>
#include <lib/ecall.h>
#include <lib/string.h>

#define UPRINTF_BUFF_SIZE 1024

byte* printf_putc(char c, byte mode, byte* ptr) {
	(void)mode;
	*ptr++ = c;
	return ptr;
}

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

void sprintf(byte* buffer, const char* format, ...) {
	va_list ap;
	va_start(ap, format);
	printf_core(MODE_BUFFER, buffer, format, ap);
	va_end(ap);
}

int32_t gets(char* buffer, uint32_t length) {
	if (buffer == NULL || length == 0) return 0;
	uart_dev_opts options;
	options.length = length;
	options.buffer = (byte*)buffer;
	return (int32_t)ecall_read(UART_DEV_NUM, (byte*)&options);
}

int8_t fcreate(const char* path) {
	disk_dev_opts options;
	options.file = NULL;
	options.mode = FILE_CREATE;
	options.buff_in = (byte*)path;
	options.buff_out = NULL;
	options.length = strlen(path);
	return ecall_open(DISK_DEV_NUM, &options);
}

int8_t fopen(const char* path, FILE* file) {
	disk_dev_opts options;
	file->fd = (FD)-1;
	file->inode = NULL;
	options.file = file;
	options.mode = FILE_OPEN;
	options.buff_in = path;
	options.buff_out = NULL;
	options.length = strlen(path);
	return ecall_open(DISK_DEV_NUM, &options);
}

int8_t fclose(FILE* file) {
	disk_dev_opts options;
	options.file = file;
	options.mode = 0; /* forgot about close when writing, is it necessary? */
	options.buff_in = NULL;
	options.buff_out = NULL;
	options.length = 0;
	return ecall_close(DISK_DEV_NUM, &options);
}

uint32_t fread(FILE* file, byte* buffer, uint32_t len) {
	disk_dev_opts options;
	options.file = file;
	options.mode = FILE_READ;
	options.buff_in = NULL;
	options.buff_out = buffer;
	options.length = len;
	return ecall_read(DISK_DEV_NUM, &options);
}

uint32_t fwrite(FILE* file, byte* buffer, uint32_t len) {
	disk_dev_opts options;
	options.file = file;
	options.mode = FILE_WRITE;
	options.buff_in = buffer;
	options.buff_out = NULL;
	options.length = len;
	return ecall_write(DISK_DEV_NUM, &options);
}

//void fdelete() {
//	// unimplemented
//}

int8_t mkdir(const char* path) {
	dirent_t placeholder; /* mkdir returns the dirent_t of the directory but nothing uses it */
	disk_dev_opts options;
	options.file = NULL;
	options.mode = DIR_CREATE;
	options.buff_in = NULL;
	options.buff_out = &placeholder;
	options.length = 0;
	return ecall_open(DISK_DEV_NUM, &options);
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
	options.buff_in = path;
	options.buff_out = (byte*)out;
	options.length = count;
	return ecall_read(UART_DEV_NUM, &options);
}

/* Function returns a full directory_t of the target dir
   If chdir is true, changes the process cwd to the output directory */
int8_t getdir(const char* path, directory_t* out, bool chdir) {
	disk_dev_opts options;
	options.file = NULL;
	options.mode = DIR_OPEN;
	options.buff_out = out;

	options.length = strlen(path);
	byte buffer[MAX_PATH_LEN + 4]; /* todo: improve this */
	memcpy(buffer, &chdir, sizeof(bool));
	memcpy(buffer + sizeof(bool), path, options.length);
	buffer[options.length] = '\0';
	options.buff_in = buffer;

	return ecall_open(DISK_DEV_NUM, &options);
}
