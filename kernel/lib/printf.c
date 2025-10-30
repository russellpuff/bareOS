#include <lib/printf.h>

static byte* put_number(uint64_t u, byte mode, byte* ptr) {
    if (u == 0) { return printf_putc('0', mode, ptr); }
    char buff[20];
    int16_t i = 0;
    while (u) { uint64_t r = u / 10; buff[i++] = '0' + (char)(u - r * 10); u = r; }
    while (i--) ptr = printf_putc(buff[i], mode, ptr);
    return ptr;
}

static byte* put_str(const char* s, byte mode, byte* ptr) {
    while (*s) ptr = printf_putc(*s++, mode, ptr);
    return ptr;
}

static byte* write_color_token(char t, byte mode, byte* ptr) {
    switch (t) {
    case '0': ptr = put_str("\x1b[0m", mode, ptr);  break; /* reset */
    case 'r': ptr = put_str("\x1b[31m", mode, ptr); break;
    case 'g': ptr = put_str("\x1b[32m", mode, ptr); break;
    case 'y': ptr = put_str("\x1b[33m", mode, ptr); break;
    case 'b': ptr = put_str("\x1b[34m", mode, ptr); break;
    case 'm': ptr = put_str("\x1b[35m", mode, ptr); break;
    case 'c': ptr = put_str("\x1b[36m", mode, ptr); break;
    case 'w': ptr = put_str("\x1b[37m", mode, ptr); break;
    case 'x': ptr = put_str("\x1b[01;32m", mode, ptr); break;
    default:  ptr = printf_putc('&', mode, ptr);
        ptr = printf_putc(t, mode, ptr);
        break;
    }
    return ptr;
}

void printf_core(byte mode, byte* ptr, const char* format, va_list ap) {
    while (*format != '\0') {
        if (*format == '%') {
            switch (*(++format)) {
            case 'd':
                int32_t d = va_arg(ap, int32_t);
                if (d < 0) {
                    ptr = printf_putc('-', mode, ptr);
                    d *= -1;
                }
                ptr = put_number((uint64_t)(uint32_t)d, mode, ptr);
                break;
            case 'u':
                uint32_t ud = va_arg(ap, uint32_t);
                ptr = put_number((uint64_t)ud, mode, ptr);
                break;
            case 'l':
                if (*(format + 1) == 'u') {
                    ++format;
                    uint64_t ul = va_arg(ap, uint64_t);
                    ptr = put_number(ul, mode, ptr);
                }
                else {
                    int64_t l = va_arg(ap, int64_t);
                    ptr = put_number((uint64_t)l, mode, ptr);
                }
                break;
            case 'x':
                uint32_t hex = va_arg(ap, uint32_t);
                ptr = printf_putc('0', mode, ptr); ptr = printf_putc('x', mode, ptr);
                if (hex == 0) { ptr = printf_putc('0', mode, ptr); break; }
                bool lead_zero = true; // Ignore leading zeroes.  
                for (uint32_t i = 0, mask = 0xF0000000; i < 8; ++i, mask >>= 4) {
                    uint32_t num = (hex & mask) >> (28 - (i * 4));
                    char c;
                    if (num < 10) { c = num + '0'; }
                    else { c = num + 'W'; }
                    if (c != '0' || !lead_zero) {
                        ptr = printf_putc(c, mode, ptr);
                        lead_zero = false; // Once we hit a non-zero number, leading zeroes are done.
                    }
                }
                break;
            case 's': // lousy %s implementation
                char* str = va_arg(ap, char*);
                while (*str != '\0') {
                    ptr = printf_putc(*str++, mode, ptr);
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
                ptr = printf_putc('%', mode, ptr);
            default: // Print nothing for now. GCC will print nothing if there's no valid character following the %
                break;
            }
        }
        else if (*format == '&') { /* OC donut steel color handler. */
            write_color_token(*++format, mode, ptr);
        }
        else {
            ptr = printf_putc(*format, mode, ptr);
        }
        ++format;
    }
}