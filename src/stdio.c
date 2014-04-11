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


static inline char* utoa(unsigned int value, char* s, int const radix) {
    char* s1 = s;
    char* s2 = s;

    do {
        *s2++ = "0123456789abcdefghijklmnopqrstuvwxyz"[value % (unsigned int)radix];
        value /= (unsigned int)radix;
    } while (value > 0);

    *s2-- = '\0';

    while (s1 < s2) {
        char const c = *s1;
        *s1++ = *s2;
        *s2-- = c;
    }

    return s;
}


char* itoa(int value, char* s, int radix) {
    unsigned int t = (unsigned int)value;
    char* ss = s;

    if (value < 0 && radix == 10) {
        *ss++ = '-';
        t = -t;
    }

    utoa(t, ss, radix);

    return s;
}


int puts(const char* s) {
    while (*s != '\0') {
        putchar(*s++);
    }

    return 1;
}


void printf(const char* format, ...) {
    va_list args;

    va_start(args, format);

    for (const char* c = format; *c != '\0'; ++c) {
        if (*c != '%') {
            putchar(*c);
            continue;
        }

        ++c;
        switch (*c) {
            /* case '\n': */
            /* putchar('\n'); */
            case 'c':
                putchar((unsigned char)va_arg(args, int));
            case 's':
                puts(va_arg(args, const char*));
                break;
            case 'd':
            case 'i':
            case 'x': {  // add local scope
                /* INT_MAX = +32767 なので最大の5桁以上のバッファを確保 */
                char buf[10];
                puts(itoa(va_arg(args, int), buf, (*c == 'x' ? 16 : 10)));
                break;
                case '%':
                    putchar('%');
            }
        }
    }

    va_end(args);
}
