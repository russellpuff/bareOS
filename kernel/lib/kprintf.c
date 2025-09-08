#include <bareio.h>
#include <barelib.h>

/* Helper function for numbers. */
void put_number(uint64 u) {
    if(u == 0) { uart_putc('0'); return; }
    char buff[20];
    int16 i = 0;
    while(u) {
        uint64 r = u / 10;
        buff[i++] = '0' + (char)(u - r * 10);
        u = r;
    }
    while(i--) uart_putc(buff[i]);
} 

void write_color_token(char t) {
    switch(t){
        case '0': uart_write("\x1b[0m");  break; // reset
        case 'r': uart_write("\x1b[31m"); break;
        case 'g': uart_write("\x1b[32m"); break;
        case 'y': uart_write("\x1b[33m"); break;
        case 'b': uart_write("\x1b[34m"); break;
        case 'm': uart_write("\x1b[35m"); break;
        case 'c': uart_write("\x1b[36m"); break;
        case 'w': uart_write("\x1b[37m"); break;
        case 'x': uart_write("\x1b[01;32m"); break; // prompt bright green
        default:  uart_putc('&'); uart_putc(t); return; // literal
    }
}

void kprintf(const char* format, ...) {
  va_list ap;
  va_start(ap, format);

  while(*format != '\0') {
    if(*format == '%') {
        switch(*(++format)) {
            case 'd':
                int32 d = va_arg(ap, int32);
                if(d < 0) {
                    uart_putc('-');
                    d *= -1;

                }
                put_number((uint64)(uint32)d);
                break;
            case 'u':
                uint32 ud = va_arg(ap, uint32);
                put_number((uint64)ud);
                break;
            case 'l':
                if(*(format + 1) == 'u') {
                    ++format;
                    uint64 ul = va_arg(ap, uint64);
                    put_number(ul);
                    break;
                } else {
                    int64 l = va_arg(ap, int64);
                    put_number((uint64)l);
                }
            case 'x':
                uint32 hex = va_arg(ap, uint32);
                uart_putc('0'); uart_putc('x');
                if(hex == 0) { uart_putc('0'); break; }
                bool lead_zero = true; // Ignore leading zeroes.  
                for(uint32 i = 0, mask = 0xF0000000; i < 8; ++i, mask >>= 4) {
                    uint32 num = (hex & mask) >> (28 - (i * 4)); 
                    char c;
                    if(num < 10) { c = num + '0'; }
                    else { c = num + 'W'; }
                    if(c != '0' || !lead_zero) {
                        uart_putc(c);
                        lead_zero = false; // Once we hit a non-zero number, leading zeroes are done.
                    }
                }
                break;
            case 's': // lousy %s implementation
                char* str = va_arg(ap, char*);
                while(*str != '\0') {
                    uart_putc(*str++);
                }
                break;
            case ' ':
                /* Edge case where % was followed by a space.
                 * When gcc compiles without warnings, behavior like "% text" will be read as "%t ext"
                 * In this case, it WILL try to use its defined behavior for %t, it will also put a space BEFORE whatever it spits out.
                 * I don't do this right now because I don't know what I want to do. 
                */
                break;
            case '%':
                uart_putc('%');
            default: // Print nothing for now. GCC will print nothing if there's no valid character following the %
                break;
        }
    } else if(*format == '&') { /* OC donut steel color handler. */
        write_color_token(*++format);
    } else {
        uart_putc(*format);
    }
    ++format;
  }  
  va_end(ap);
}
