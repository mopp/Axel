/************************************************************
 * File: graphic.c
 * Description: graphic code.
 ************************************************************/

#include <graphic.h>
#include <graphic_txt.h>
#include <graphic_vbe.h>


void clean_screen(void) {
#ifdef TEXT_MODE
    clean_screen_txt();
#else
    RGB8 c;
    c.r = 0xFF;
    c.g = 0x00;
    c.b = 0x1F;
    clean_screen_g(&c);
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


/* TODO: split printf to stdio */
void printf(const char* format, ...) {
    va_list args;

#ifdef TEXT_MODE
/* printf_txt(format, args); */
#else
#endif

    va_end(args);
}
