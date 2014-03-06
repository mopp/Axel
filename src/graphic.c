/************************************************************
 * File: graphic.c
 * Description: graphic code.
 ************************************************************/

#include <graphic.h>


void clean_screen(void) {
#ifdef TEXT_MODE
    clean_screen_txt();
#else
#endif
}


int putchar(int c) {
#ifdef TEXT_MODE
    return putchar_txt(c);
#else
#endif
}


const char* puts(const char* str) {
#ifdef TEXT_MODE
    return puts_txt(str);
#else
#endif
}


void printf(const char* format, ...) {
    va_list args;

#ifdef TEXT_MODE
    printf_txt(format, args);
#else
#endif

    va_end(args);
}
