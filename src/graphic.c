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


void clean_screen(RGB8 const* const c) {
#ifdef TEXT_MODE
    c;
    clean_screen_txt();
#else
    clean_screen_vbe(c);
#endif
}


int putchar(int c) {
#ifdef TEXT_MODE
    return putchar_txt(c);
#else
    return putchar_vbe(c);
#endif
}
