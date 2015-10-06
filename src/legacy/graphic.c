/************************************************************
 * File: graphic.c
 * Description: graphic code.
 ************************************************************/

#include <graphic.h>
#include <graphic_txt.h>
#include <graphic_vbe.h>

Axel_state_code init_graphic(Multiboot_info const * const info) {
#ifdef TEXT_MODE
    return AXEL_SUCCESS;
#else
    return init_graphic_vbe(info);
#endif
}


void clean_screen(RGB8 const* const c) {
#ifdef TEXT_MODE
    c;  // dummy.
    clean_screen_txt();
#else
    clean_screen_vbe(c);
#endif
}


#if 0
int putchar(int c) {
#ifdef TEXT_MODE
    return putchar_txt(c);
#else
    return putchar_vbe(c);
#endif
}
#endif
