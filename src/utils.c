/**
 * @file utils.c
 * @brief utility implementations.
 * @author mopp
 * @version 0.1
 * @date 2014-09-27
 */


#include <utils.h>
#include <stdbool.h>
#include <stdint.h>

/* #pragma GCC diagnostic ignored "-Wcast-qual" */



void* memchr(void const* s, register int c, size_t n) {
    register unsigned char const* p = (unsigned char const*)s;
    register unsigned char const* const lim = (unsigned char const*)s + n;
    if (s == NULL || n == 0) {
        return NULL;
    }

    do {
        if (*p == (unsigned char)c) {
            return (void*)(uintptr_t)p;
        }
    } while (p < lim);

    return NULL;
}


int memcmp(void const* s1, void const* s2, size_t n) {
    int result = 0;
    size_t nr, t = 0;

    __asm__ volatile("cld");

    if (4 <= n) {
        nr = n >> 2;
        __asm__ volatile(
            "repe cmpsl             \n"
            "movl -4(%%esi), %%eax  \n"
            "movl -4(%%edi), %%ecx  \n"
            "subl %%ecx, %%eax      \n"
            "movl %%eax, %[r]       \n"
            : [r] "=m"(result)
            : "S"((void*)((uintptr_t)s1 + t)), "D"((void*)((uintptr_t)s2 + t)), "c"(nr)
            : "memory", "%esi", "%edi", "%ecx", "%eax"
        );
        t = nr << 2;
        n -= t;
    }

    if (2 <= n && result == 0) {
        nr = n >> 1;
        __asm__ volatile(
            "repe cmpsw             \n"
            "xor %%eax, %%eax       \n"
            "xor %%ecx, %%ecx       \n"
            "movw -2(%%esi), %%ax   \n"
            "movw -2(%%edi), %%cx   \n"
            "subl %%ecx, %%eax      \n"
            "movl %%eax, %[r]       \n"
            : [r] "=m"(result)
            : "S"((void*)((uintptr_t)s1 + t)), "D"((void*)((uintptr_t)s2 + t)), "c"(nr)
            : "memory", "%esi", "%edi", "%ecx", "%eax"
        );
        t = nr << 1;
        n -= t;
    }

    if (n != 0 && result == 0) {
        __asm__ volatile(
            "repe cmpsb             \n"
            "xor %%eax, %%eax       \n"
            "xor %%ecx, %%ecx       \n"
            "movb -1(%%esi), %%al   \n"
            "movb -1(%%edi), %%cl   \n"
            "subl %%ecx, %%eax      \n"
            "movl %%eax, %[r] \n"
            : [r] "=m"(result)
            : "S"((void*)((uintptr_t)s1 + t)), "D"((void*)((uintptr_t)s2 + t)), "c"(n)
            : "memory", "%esi", "%edi", "%ecx", "%eax"
        );
    }

    return result;
}


void* memcpy(void* restrict b1, const void* restrict b2, size_t n) {
    uint8_t* restrict p1 = b1;
    uint8_t const* restrict p2 = b2;
    size_t nr, t;

    __asm__ volatile("cld");

    nr = n / 4;
    if (nr != 0) {
        __asm__ volatile(
                "rep movsd  \n"
                :
                : "S"(p2), "D"(p1), "c"(nr)
                : "memory", "%esi", "%edi", "%ecx"
        );
        t = nr * 4;
        n -= t;
        p1 += t;
        p2 += t;
    }


    nr = n / 2;
    if (nr != 0) {
        __asm__ volatile(
                "rep movsw   \n"
                :
                : "S"(p2), "D"(p1), "c"(nr)
                : "memory", "%esi", "%edi", "%ecx"
        );
        t = nr * 2;
        n -= t;
        p1 += t;
        p2 += t;
    }

    __asm__ volatile(
            "rep movsb   \n"
            :
            : "S"(p2), "D"(p1), "c"(n)
            : "memory", "%esi", "%edi", "%ecx"
    );

    return b1;
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
    register const unsigned char *ss1, *ss2;
    for (ss1 = (const unsigned char*)s1, ss2 = (const unsigned char*)s2; *ss1 == *ss2 && *ss1 != '\0'; ss1++, ss2++) {
    }
    return *ss1 - *ss2;
}


char* strcpy(register char* s1, register char const* s2) {
    char* p = s1;
    while (*s2 != '\0') {
        *s1++ = *s2++;
    }
    *s1 = *s2;

    return p;
}


size_t strlen(register const char* s) {
    if (s == NULL) {
        return 0;
    }

    register size_t i = 0;
    do {
        ++i;
    } while (*s++ != '\0');

    return i - 1;
}


char* strchr(register char const* s, register int c) {
    if (s == NULL) {
        return NULL;
    }

    while (*s != c && *s != '\0') {
        s++;
    }

    return (*s == '\0') ? NULL : (char*)(uintptr_t)s;
}


char* strrchr(const char* s, int c) {
    if (s == NULL) {
        return NULL;
    }

    char* found = NULL;
    while (*s != '\0') {
        if (*s == c) {
            found = (char*)(uintptr_t)s;
        }
        ++s;
    }

    return found;
}


char* strstr(char const* s1, char const* s2) {
    if ((s1 == NULL) || (s2 == NULL)) {
        return NULL;
    }

    size_t l1 = strlen(s1);
    size_t l2 = strlen(s2);
    if (l2 == 0) {
        return (char*)(uintptr_t)s1;
    }

    if (l1 < l2) {
        return NULL;
    }

    while (*s1 != '\0') {
        if (memcmp(s1, s2, l2) == 0) {
            return (char*)(uintptr_t)s1;
        }
        ++s1;
    }

    return NULL;
}


size_t trim_tail(char* s) {
    if (s == NULL) {
        return 0;
    }

    size_t i, len = strlen(s);
    i = len;
    do {
        i--;
    } while (0 != i && s[i] == ' ');

    s[i + 1] = '\0';

    return len - i;
}


char* utoa(uint32_t value, char* s, uint8_t const radix) {
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
