/**
 * @file include/graphic_vbe.h
 * @brief Graphic Header for VBE2.0+.
 * @author mopp
 * @version 0.1
 * @date 2014-10-13
 */


#ifndef _GRAPHIC_VBE_H_
#define _GRAPHIC_VBE_H_



#include <rgb8.h>
#include <state_code.h>
#include <multiboot.h>


extern void clean_screen_vbe(RGB8 const* const);
extern Axel_state_code init_graphic_vbe(Multiboot_info const * const);
extern int putchar_vbe(int);



#endif
