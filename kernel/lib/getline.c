#include <lib/barelib.h>
#include <lib/bareio.h>
#include <device/tty.h>

/* Gets a line from the UART / TTY and stores it into a buffer, converts the line into a C string. */
/* Returns the number of characters read. */
uint32_t get_line(char* buffer, uint32_t size) {
	if(size == 0) return 0;
	char* ptr = buffer;
	char* end = buffer + size - 1;
	while(1) {
		char ch = getc();
		switch(ch) {
			case '\r':
			case '\n':
				putc('\r');
				putc('\n');
				*ptr = '\0';
				return (uint32_t)(ptr - buffer);
			case '\b':
			case 0x7f:
				if (ptr > buffer) { tty_bkspc(); --ptr; }
				break;
			case 0x1b: /* Kill pesky ANSI sequence junk */
				uint8_t a = (uint8_t)getc();
				if (a == '[') {
					while (1) {
						a = (uint8_t)getc();
						if (a >= 0x40 && a <= 0x7E) break; /* End of ANSI sequence */
					}
				}
				else if (a == 'O') { (void)getc(); }
				else {
					/* Handle future ESC x sequences if necessary. If just ESC, it's gone. */
				}
			default:
				if (ch < 0x20 || ch > 0x7E) break; /* Consume unspecified nonprint characters. */
				putc(ch);
				if (ptr < end) *ptr++ = ch;
				break;
		}
		if (ptr == end) return (uint32_t)(end - buffer);
	}
}
