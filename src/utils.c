/**
 * @file utils.c
 * @brief utility implementations.
 * @author mopp
 * @version 0.1
 * @date 2014-09-27
 */


#include <utils.h>
#include <stdarg.h>
#include <graphic.h>


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
    const unsigned char* ucb1, *ucb2, *t;
    int result = 0;
    ucb1 = buf1;
    ucb2 = buf2;
    t = ucb2 + n;

    while (t != buf2 && result == 0) {
        result = *ucb1++ - *ucb2++;
    }

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


int strcmp(const char* s1, const char* s2) {
    int result = 0;

    while (*s1 != '\0' && *s2 == '\0' && result == 0) {
        result = *s1++ - *s2++;
    }

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
