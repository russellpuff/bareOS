#include <lib/barelib.h>
#include <lib/bareio.h>
#include <mm/malloc.h>
#include <device/tty.h>

#define CSI_MAX_CHARS 32
#define OSC_MAX_CHARS 256
#define ESC_INTERMEDIATE_MAX 8
bool RAW_GETLINE = false; /* Determines whether we run get_line in new "swallow" mode or old "manual filter" mode */

/* Raw line getting, takes all characters then filters out nonprint for the caller
   This is for debugging purposes and should eventually be removed                 */
uint32_t get_raw_line(char* buffer, uint32_t size) {
	if (size == 0) return 0;
	char* raw_in = malloc(size * 2);
	char* ptr = raw_in;
	char* end = raw_in + (size * 2) - 1;
	bool doit = true;
	while (doit) {
		char ch = getc();
		switch (ch) {
		case '\r':
		case '\n':
			putc('\r');
			putc('\n');
			*ptr = '\0';
			doit = false;
			break;
		case '\b':
		case 0x7f:
			if (ptr > raw_in) { 
				tty_bkspc(); 
				--ptr; 
			}
			break;
		default:
			if (ptr < end) {
				*ptr++ = ch;
				putc(ch);
			}
			break;
		}
		if (ptr == end) doit = false;
	}

	uint32_t copied = 0;
	ptr = raw_in;
	while (copied < size - 1 && ptr < end && *ptr != '\0') {
		uint8_t c = (uint8_t)*ptr++;
		if (c >= 0x20 && c <= 0x7E) {
			buffer[copied++] = (char)c;
		}
	}

	free(raw_in);
	buffer[copied] = '\0';
	return copied;
}

/*
 * Consume the trailing bytes of an ANSI control sequence (CSI) until the final byte is
 * observed or the parser decides the stream is junk. The guard prevents us from blocking
 * forever if an automated testing client truncates the sequence mid-stream.
 */
static void swallow_csi(void) {
	for (uint32_t i = 0; i < CSI_MAX_CHARS; ++i) {
		uint8_t c = (uint8_t)getc();
		if (c >= 0x40 && c <= 0x7E) return; /* Final byte terminator */
	}
}

/* Intermediate/charset escape sequences are short (ESC ( B). Skip bytes until a final byte
 * arrives or the safety limit trips so we never wedge the shell on malformed input. */
static void swallow_intermediate(void) {
	for (uint32_t i = 0; i < ESC_INTERMEDIATE_MAX; ++i) {
		uint8_t c = (uint8_t)getc();
		if (c >= 0x30 && c <= 0x7E) return;
	}
}

/*
 * OSC/SS2/SS3 style strings terminate with BEL or ST (ESC \). While parsing we may run into
 * embedded CSI/SS3 sequences. The bounds keep malformed streams from locking the shell.
 */
static void swallow_osc_stream(void) {
	for (uint32_t count = 0; count < OSC_MAX_CHARS; ++count) {
		uint8_t b = (uint8_t)getc();
		if (b == 0x07) return; /* BEL terminator */
		if (b == 0x1b) {
			uint8_t c = (uint8_t)getc();
			if (c == '\\') return; /* ESC \ terminator */
			if (c == '[') {
				swallow_csi();
				continue;
			}
			else if (c == 'O') {
				(void)getc();
				continue;
			}
			else if (c >= 0x20 && c <= 0x2F) {
				swallow_intermediate();
				continue;
			}
		}
	}
}

uint32_t get_line(char* buffer, uint32_t size) {
	if (RAW_GETLINE) return get_raw_line(buffer, size);
	if (size == 0) return 0;

	char* ptr = buffer;
	char* end = buffer + size - 1;
	while (1) {
		char ch = getc();
		switch (ch) {
		case '\r':
		case '\n':
			putc('\r');
			putc('\n');
			*ptr = '\0';
			return (uint32_t)(ptr - buffer);
		case '\b':
		case 0x7f:
			if (ptr > buffer) {
				tty_bkspc();
				--ptr;
			}
			break;
		case '\t':
			putc('\t');
			if (ptr < end) {
				*ptr++ = '\t';
			}
			break;
		case 0x1b: {	/* Consume ANSI/VT escape sequences before they leak into the buffer. */
			/*
			 * Automated testing environments sometimes rely on cursor-control
			 * sequences to keep the shell usable. Swallow the common CSI, SS3, and OSC-style
			 * streams so those bytes never populate the caller buffer; only printable characters
			 * escape this block.
			 * 
			 * If you use proc.stdin.write() in python tests, you can get around this problem
			 * but you won't be able to use the normal scons build mechanism without a heavy
			 * test integration
			 */
			uint8_t a = (uint8_t)getc();
			switch (a) {
				case '[': 
					swallow_csi(); 
					continue;
				case 'O': 
					(void)getc(); 
					continue;
				case ']':
				case 'P':
				case 'X':
				case '^':
				case '_':
					swallow_osc_stream();
					continue;
				default:
					if (a >= 0x20 && a <= 0x2F) {
						swallow_intermediate();
					}
					continue;
			}
		}
		default:
			if (ch < 0x20 || ch > 0x7E) break; /* Consume unspecified nonprint characters. */
			putc(ch);
			if (ptr < end) {
				*ptr++ = ch;
			}
			break;
		}
		if (ptr == end) {
			return (uint32_t)(end - buffer);
		}
	}
}
