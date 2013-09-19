/************************************************************
 * File: string.c
 * Description: String manipulate functions.
 ************************************************************/

#include <stdint.h>
#include <string.h>


/* 数字を文字に変換 TODO: 負の数に対応 */
char* itoa(char* buf, char base, uint32_t num) {
    int i, divisor = 16;

    if (base == 'd') {
        divisor = 10;
    } else if (base == 'x') {
        *buf++ = '0';
        *buf++ = 'x';
    }

    int remainder;
    do {
        remainder = num % divisor;
        buf[i++] = (remainder < 10) ? ('0' + remainder) : ('A' + remainder - 10);
    } while (num /= divisor);

    /* 終端子追加 */
    buf[i] = '\0';

    /* 反転 */
    char *p1, *p2;
    char tmp;
    p1 = buf;
    p2 = buf + i - 1;
    while (p1 < p2) {
        tmp = *p1;
        *p1 = *p2;
        *p2 = tmp;

        p1++;
        p2--;
    }

    return buf;
}

uint32_t strlen(const char* str) {
    uint32_t i = 0;
    while (*str++ != '\0') {
        ++i;
    }

    return i;
}
