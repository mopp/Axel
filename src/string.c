/************************************************************
 * File: string.c
 * Description: String manipulate functions.
 ************************************************************/

#include <stdint.h>
#include <string.h>
#include <graphic_txt.h>


size_t strlen(const char* str) {
    size_t i = 0;

    while (*str++ != '\0') {
        ++i;
    }

    return i;
}
