#include <bareio.h>
#include <barelib.h>

#define MODE_REGULAR 0
#define MODE_BUFFER 1
//#define MODE_FILE 2

/* Helper functions. mode_put is the only thing that can advance the buffer pointer so always return it for updates. */
byte* mode_put(char c, byte mode, byte* ptr) {
    switch(mode) {
        case MODE_REGULAR: uart_putc(c); break;
        case MODE_BUFFER: *ptr++ = c; break;
    }
    return ptr;
}

byte* put_number(uint64 u, byte mode, byte* ptr) {
    if(u == 0) { ptr = mode_put('0', mode, ptr); return ptr; }
    char buff[20];
    int16 i = 0;
    while(u) {
        uint64 r = u / 10;
        buff[i++] = '0' + (char)(u - r * 10);
        u = r;
    }
    while(i--) ptr = mode_put(buff[i], mode, ptr);
    return ptr;
} 

byte* write_color_token(char t, byte mode, byte* ptr) {
    switch(t){
        case '0': uart_write("\x1b[0m");  break; // reset
        case 'r': uart_write("\x1b[31m"); break; // red
        case 'g': uart_write("\x1b[32m"); break; // green
        case 'y': uart_write("\x1b[33m"); break; // yellow
        case 'b': uart_write("\x1b[34m"); break; // blue
        case 'm': uart_write("\x1b[35m"); break; // magenta
        case 'c': uart_write("\x1b[36m"); break; // cyan
        case 'w': uart_write("\x1b[37m"); break; // white
        case 'x': uart_write("\x1b[01;32m"); break; // prompt bright green
        default: // literal
            ptr = mode_put('&', mode, ptr); 
            ptr = mode_put(t, mode, ptr); 
            break; 
    }
    return ptr;
}

/* Idk what to call this lmao. All kprintf calls route to this. */
void master_kprintf(byte mode, byte* ptr, const char* format, va_list ap) {
    while(*format != '\0') {
        if(*format == '%') {
            switch(*(++format)) {
                case 'd':
                    int32 d = va_arg(ap, int32);
                    if(d < 0) {
                        ptr = mode_put('-', mode, ptr);
                        d *= -1;
                    }
                    ptr = put_number((uint64)(uint32)d, mode, ptr);
                    break;
                case 'u':
                    uint32 ud = va_arg(ap, uint32);
                    ptr = put_number((uint64)ud, mode, ptr);
                    break;
                case 'l':
                    if(*(format + 1) == 'u') {
                        ++format;
                        uint64 ul = va_arg(ap, uint64);
                        ptr = put_number(ul, mode, ptr);
                    } else {
                        int64 l = va_arg(ap, int64);
                        ptr = put_number((uint64)l, mode, ptr);
                    }
                    break;
                case 'x':
                    uint32 hex = va_arg(ap, uint32);
                    ptr = mode_put('0', mode, ptr); ptr = mode_put('x', mode, ptr);
                    if(hex == 0) { ptr = mode_put('0', mode, ptr); break; }
                    bool lead_zero = true; // Ignore leading zeroes.  
                    for(uint32 i = 0, mask = 0xF0000000; i < 8; ++i, mask >>= 4) {
                        uint32 num = (hex & mask) >> (28 - (i * 4)); 
                        char c;
                        if(num < 10) { c = num + '0'; }
                        else { c = num + 'W'; }
                        if(c != '0' || !lead_zero) {
                            ptr = mode_put(c, mode, ptr);
                            lead_zero = false; // Once we hit a non-zero number, leading zeroes are done.
                        }
                    }
                    break;
                case 's': // lousy %s implementation
                    char* str = va_arg(ap, char*);
                    while(*str != '\0') {
                        ptr = mode_put(*str++, mode, ptr);
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
                    ptr = mode_put('%', mode, ptr);
                default: // Print nothing for now. GCC will print nothing if there's no valid character following the %
                    break;
            }
        } else if(*format == '&') { /* OC donut steel color handler. */
            write_color_token(*++format, mode, ptr);
        } else {
            ptr = mode_put(*format, mode, ptr);
        }
    ++format;
  }  
}

void kprintf(const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    master_kprintf(MODE_REGULAR, NULL, format, ap);
    va_end(ap);
}

void ksprintf(byte* buff, const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    master_kprintf(MODE_BUFFER, buff, format, ap);
    va_end(ap);
}
