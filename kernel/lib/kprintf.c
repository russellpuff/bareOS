#include <lib/bareio.h>
#include <mm/vm.h>
#include <dev/printf.h>
#include <barelib.h>

/* Helper functions. printf_putc is the only thing that can advance the buffer pointer so always return it for updates */
byte* printf_putc(char c, uint8_t mode, byte* ptr) {
	switch(mode) {
		case MODE_REGULAR: putc(c); break;
		case MODE_BUFFER: *ptr++ = c; break;
		case MODE_RAW: /* temporary implementation */
			#define UART0_CFG_REG 0x10000000
			#define UART0_RW_REG   0x0 
			#define UART0_STAT_REG 0x5
			#define UART_IDLE     0x20
			byte* uart = (byte*)PA_TO_KVA(UART0_CFG_REG);
			while ((uart[UART0_STAT_REG] & UART_IDLE) == 0); /*  Wait for the UART to be idle */
			uart[UART0_RW_REG] = (c & 0xff);                 /*  Send character to the UART   */
			break;
	}
	return ptr;
}

void kprintf(const char* format, ...) {
	va_list ap;
	va_start(ap, format);
	printf_core(MODE_REGULAR, NULL, format, ap);
	va_end(ap);
}

void ksprintf(byte* buff, const char* format, ...) {
	va_list ap;
	va_start(ap, format);
	printf_core(MODE_BUFFER, buff, format, ap);
	va_end(ap);
}

/* Kernel raw printf, bypasses tty to talk to the uart directly. Only for panic() and debug. */
void krprintf(const char* format, ...) {
	va_list ap; va_start(ap, format);
	vkrprintf(format, ap);
	va_end(ap);
}

/* Only panic() should use this */
void vkrprintf(const char* format, va_list ap) {
	printf_core(MODE_RAW, NULL, format, ap);
}

/* Wrapper for below */
static void kvprintf(uint8_t mode, byte* buff, const char* format, va_list ap) {
	printf_core(mode, buff, format, ap);
}

/* Kernel-side implementation of printf, just routes to appropriate kprint version. 
   Prefer not to use these in kernel code, only shared libraries.                   */
void printf(const char* format, ...) {
	va_list ap;
	va_start(ap, format);
	kvprintf(MODE_REGULAR, NULL, format, ap);
	va_end(ap);
}

void sprintf(byte* buff, const char* format, ...) {
	va_list ap;
	va_start(ap, format);
	kvprintf(MODE_BUFFER, buff, format, ap);
	va_end(ap);
}
