/************************************************************
 * File: string.c
 * Description: String manipulate functions.
 ************************************************************/

#include <string.h>


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


void* memset(void* buf, int ch, size_t n) {
    unsigned char* t = (unsigned char*)buf;

    while (0 < n--) {
        *t++ = (unsigned char const)ch;
    }

    return buf;
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
