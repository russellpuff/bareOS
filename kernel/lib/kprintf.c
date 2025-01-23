#include <bareio.h>
#include <barelib.h>

void kprintf(const char* format, ...) {
  va_list ap;
  va_start(ap, format);

  while(*format != '\0') {
    if(*format == '%') {
        switch(*(++format)) {
            case 'd':
                int dec = va_arg(ap, int);
                bool neg = false;
                if(dec < 0) { neg = true; dec *= -1; }
                /* Without resorting to an array, this annoyingly runs O(2n)
                   Wait wouldn't using an array still be O(2n?) lol */
                int rev = 0;
                while(dec > 0) {
                    int digit = dec % 10;
                    rev = (rev * 10) + digit;
                    dec /=10;
                }
                if(neg) { uart_putc('-'); }
                while(rev > 0) {
                    int digit = rev % 10;
                    uart_putc(digit + '0');
                    rev /= 10;
                }
                break;
            case 'x':
                uint hex = va_arg(ap, uint);
                uart_putc('0'); uart_putc('x');
                uint mask = 0xF0000000;
                if(hex == 0) { uart_putc('0'); break; }
                bool lead_zero = true; // This is to ignore leading zeroes since the example doesn't want them. 
                for(int i = 0; i < 8; ++i) {
                    uint num = (hex & mask) >> (28 - (i * 4)); 
                    char c;
                    if(num < 10) { c = num + '0'; }
                    else { c = num + 'W'; }
                    if(c != '0' || !lead_zero) {
                        uart_putc(c);
                        lead_zero = false; // Once we hit a non-zero number, leading zeroes are done. 
                    }
                    mask >>= 4; // apparently >>= is an operator
                }
                break;
            case ' ':
                /* Edge case where % was followed by a space.
                 * When gcc compiles without warnings, behavior like "% text" will be read as "%t ext"
                 * In this case, it WILL try to use its defined behavior for %t, it will also put a space BEFORE whatever it spits out.
                 * I don't do this right now because I don't know what I want to do. 
                */
                uart_putc(' ');
                break;
            default: // Print nothing for now. GCC will print nothing if there's no valid character following the %
                break;
        }
    } else {
        uart_putc(*format);
    }
    ++format;
  }  
  va_end(ap);
}