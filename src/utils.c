/**
 * @file utils.c
 * @brief utility implementations.
 * @author mopp
 * @version 0.1
 * @date 2014-09-27
 */


#include <utils.h>
#include <stdarg.h>
#include <stdbool.h>
#include <window.h>


void* memchr(const void* s, int c, size_t n) {
    unsigned char const* p = (unsigned char const*)s;

    for (int i = 0; i < n; i++) {
        if (p[i] == (unsigned char)c) {
            return (void*)&p[i];
        }
    }

    return NULL;
}


int memcmp(const void* buf1, const void* buf2, size_t n) {
    const unsigned char* ucb1, *ucb2;
    int result = 0;
    ucb1 = buf1;
    ucb2 = buf2;
    unsigned char const * const limit = (char*)buf1 + n;

    do {
        result = *ucb1++ - *ucb2++;
    } while ((ucb1 < limit) && result == 0);

    return result;
}


void* memcpy(void* buf1, const void* buf2, size_t n) {
    char* p1 = (char*)buf1;
    char const* p2 = (char const*)buf2;

    while (0 < n--) {
        *p1 = *p2;
        ++p1;
        ++p2;
    }

    return buf1;
}


void* memset(void* s, register int c, size_t n) {
    register uintptr_t itr = (uintptr_t)s;
    register uintptr_t end = itr + n;

    for (register uint32_t v = (uint32_t)((c << 24) | (c << 16) | (c << 8) | c); itr < end; itr += sizeof(uint32_t)) {
        *(uint32_t*)(itr) = v;
    }

    for (register uint16_t v = (uint16_t)((c << 8) | c); itr < end; itr += sizeof(uint16_t)) {
        *(uint16_t*)(itr) = v;
    }

    for (register uint8_t v = (uint8_t)c; itr < end; itr += sizeof(uint8_t)) {
        *(uint8_t*)(itr) = v;
    }

    return s;
}


int strcmp(register const char* s1, register const char* s2) {
    int result = 0;

    do {
        result = *s1++ - *s2++;
    } while (*s1 != '\0' && *s2 != '\0' && result == 0);

    return result;
}


char* strcpy(char* s1, const char* s2) {
    char* p = s1;
    while (*s2 == '\0') {
        *s1++ = *s2++;
    }
    return p;
}


size_t strlen(const char* str) {
    size_t i = 0;

    while (*str++ != '\0') {
        ++i;
    }

    return i;
}


static inline char* utoa(uint32_t value, char* s, uint8_t const radix) {
    char* s1 = s;
    char* s2 = s;

    do {
        *s2++ = "0123456789abcdefghijklmnopqrstuvwxyz"[value % radix];
        value /= radix;
    } while (value > 0);

    *s2-- = '\0';

    while (s1 < s2) {
        char const c = *s1;
        *s1++ = *s2;
        *s2-- = c;
    }

    return s;
}


char* itoa_big(int32_t value, char* s, uint8_t radix) {
    uint32_t t = (uint32_t)value;
    char* ss = s;

    if (radix == 10 && value < 0) {
        *ss++ = '-';
        t = -t;
    }

    utoa(t, ss, radix);

    return s;
}


char* itoa(int value, char* s, int radix) {
    return itoa_big(value, s, (uint8_t)radix);
}


int isdigit(int c) {
    return ('0' <= c && c <= '9') ? 1 : 0;
}


int putchar(int c) {
    w_putchar(c);
    return 1;
}


int puts(const char* s) {
    int n = 0;

    while (*s != '\0') {
        putchar(*s++);
        ++n;
    }

    return n;
}


static inline void print_spaces(size_t n) {
    for (size_t i = 0; i < n; i++) {
        putchar(' ');
    }
}


static inline void print_zeros(size_t n) {
    for (size_t i = 0; i < n; i++) {
        putchar('0');
    }
}


static inline size_t print(char const* c, size_t width, bool is_left_align, bool is_zero_fill) {
    size_t n = 0;
    void (*f)(size_t n) = (is_zero_fill == true ? print_zeros : print_spaces);

    if (is_left_align == true) {
        n += (size_t)puts(c);
        if (width != 0 && n < width) {
            f(width - n);
        }
    } else {
        size_t s = strlen(c);
        if (width != 0 && s < width) {
            f(width - s);
        }
        n += (size_t)puts(c);
    }

    return n;
}


int printf(const char* format, ...) {
    va_list args;
    va_start(args, format);

    int n = 0;

    for (char const* c = format; *c != '\0'; ++c) {
        if (*c != '%') {
            putchar(*c);
            ++n;
            continue;
        }
        ++c;

        /* Check flag */
        bool is_left_align = false;
        bool is_exist_flag = false;
        bool is_zero_fill  = false;
        switch (*c) {
            case '-':
                /* left aligned */
                is_left_align = true;
                break;
            case '+':
                /* allways print sign */
                putchar('+');
                break;
            case ' ':
                /* TODO: */
                break;
            case '#':
                /* TODO: */
                break;
            case '0':
                is_zero_fill = true;
                break;
            default :
                is_exist_flag = true;
                break;
        }

        if (is_exist_flag == false) {
            ++c;
        }

        /* Check minimal number of output characters. */
        size_t num_len = 0;
        while (isdigit(c[num_len]) == 1) {
            ++num_len;
        }

        size_t width = 0;
        if (0 < num_len) {
            size_t base = 1;
            for (char const* i = &c[num_len - 1]; c <= i; --i, base *= 10) {
                width += ((size_t)*i - (size_t)'0') * base;
            }
            c += num_len;
        } else {
            width = 1;
        }

        /* TODO: Check length modifier. */
        if (*c == 'h') {
            /* short */
            ++c;
            if (*c == 'h') {
                /* char */
                ++c;
            }
        } else if (*c == 'l') {
            /* long or wchar_t or double */
            ++c;
            if (*c == 'l') {
                /* long long */
                ++c;
            }
        } else if (*c == 'j') {
            /* intmax_t */
            ++c;
        } else if (*c == 'z') {
            /* size_t */
            ++c;
        } else if (*c == 't') {
            /* ptrdiff_t */
            ++c;
        } else if (*c == 'L') {
            /* long double */
            ++c;
        }

        char buf[11];
        uint8_t r = 0;
        char const* str = NULL;
        switch (*c) {
            case 'c':
                if (is_left_align == true) {
                    print_spaces(width - 1);
                    putchar((unsigned char)va_arg(args, int));
                } else {
                    putchar((unsigned char)va_arg(args, int));
                    print_spaces(width - 1);
                }

                n += (width == 0) ? (1) : (width - 1);
                break;
            case 's':
                str = va_arg(args, char const*);
                n += print(str, width, is_left_align, is_zero_fill);
                break;
            case 'p':
                puts("0x");
            case 'x':
            case 'X':
            case 'u':
                r = ((*c == 'x' || *c == 'X' || *c == 'p') ? 16 : 10);
                str = utoa(va_arg(args, uint32_t), buf, r);
                n += print(str, width, is_left_align, is_zero_fill);
                break;
            case 'd':
            case 'i':
                str = itoa_big(va_arg(args, int32_t), buf, 10);
                n += print(str, width, is_left_align, is_zero_fill);
                break;
            case '%':
                putchar('%');
                break;
            case 'f':
            case 'g':
                /* TODO: */
                break;
        }
    }

    va_end(args);

    return n;
}
