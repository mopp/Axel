/************************************************************
 * File: string.c
 * Description: String manipulate functions.
 ************************************************************/

#include <stdint.h>
#include <string.h>
#include <graphic_txt.h>

/* 数字を文字に変換 TODO: 負の数に対応 */
char* itoa(char* buf, char base, int32_t num) {
    int i = 0, divisor = 16;
    /* 接頭辞をつけて、正しく返すため別のポインタで操作する */
    char* local_buf = buf;

    if (base == 'd') {
        divisor = 10;
        if (num < 0) {
            num *= -1;
            *local_buf++ = '-';
        }
    } else if (base == 'x') {
        *local_buf++ = '0';
        *local_buf++ = 'x';
    }

    int remainder;
    do {
        remainder = num % divisor;
        local_buf[i++] = (remainder < 10) ? ('0' + remainder) : ('A' + remainder - 10);
    } while (num /= divisor);

    /* 終端子追加 */
    local_buf[i] = '\0';

    /* 反転 */
    char *p1, *p2;
    char tmp;
    p1 = local_buf;
    p2 = local_buf + i - 1;
    while (p1 < p2) {
        tmp = *p1;
        *p1 = *p2;
        *p2 = tmp;

        p1++;
        p2--;
    }

    return buf;
}

size_t strlen(const char *str){
    size_t i = 0;

    while (*str++ != '\0') {
        ++i;
    }

    return i;
}
