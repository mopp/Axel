/************************************************************
 * File: graphic.c
 * Description: graphic code.
 ************************************************************/

#include <graphic.h>
#include <graphic_txt.h>
#include <graphic_vbe.h>

Axel_state_code init_graphic(Vbe_info_block const* const in, Vbe_mode_info_block const* const mi) {
#ifdef TEXT_MODE
#else
    return init_graphic_vbe(in, mi);
#endif
}


void clean_screen(void) {
#ifdef TEXT_MODE
    clean_screen_txt();
#else
    RGB8 c;
    c.r = 0xCD;
    c.g = 0x00;
    c.b = 0xAA;
    c.rsvd = 0x00;
    clean_screen_vbe(&c);
#endif
}


int putchar(int c) {
#ifdef TEXT_MODE
    return putchar_txt(c);
#else
    return c;
#endif
}


const char* puts(const char* str) {
#ifdef TEXT_MODE
    return puts_txt(str);
#else
    return str;
#endif
}
