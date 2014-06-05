/****************************************************************************************
 * @file stdio.c
 * @brief standard io functions in stl of C by mopp.
 * @author mopp
 * @version 0.1
 * @date 2014-03-17
 ****************************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <graphic.h>
#include <stdint.h>
#include <macros.h>


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


int puts(const char* s) {
    while (*s != '\0') {
        putchar(*s++);
    }

    return 1;
}

int printf(const char* format, ...) {
    va_list args;
    /* string buffer */
    char buf[11];
    /* char len_modif = 0; */
    uint8_t r = 0;

    va_start(args, format);

    for (const char* c = format; *c != '\0'; ++c) {
        if (*c != '%') {
            putchar(*c);
            continue;
        }
        ++c;

        /* check length modifier. */
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

        switch (*c) {
            case 'c':
                putchar((unsigned char)va_arg(args, int));
            case 's':
                puts(va_arg(args, const char*));
                break;
            case 'd':
            case 'i':
                puts(itoa_big(va_arg(args, int), buf, 10));
                break;
            case 'x':
            case 'X':
            case 'u':
            case 'p':
                r = ((*c == 'x' || *c == 'X') ? 16 : 10);
                puts(utoa(va_arg(args, uint32_t), buf, (*c == 'x' ? 16 : 10)));
                break;
            case '%':
                putchar('%');
        }
    }

    va_end(args);

    // TODO
    return 0;
}
