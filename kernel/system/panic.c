#include <system/panic.h>
#include <lib/bareio.h>

void panic(const char* format, ...) {
	va_list ap; 
	va_start(ap, format);
	vkrprintf(format, ap);
	va_end(ap);
	while (1);
}