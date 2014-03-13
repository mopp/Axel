/************************************************************
 * File: include/graphic_vbe.h
 * Description: Graphic Header for VBE2.0+.
 ************************************************************/

#include <rgb8.h>
#include <state_code.h>
#include <stdint.h>
#include <vbe.h>


extern void clean_screen_vbe(RGB8 const* const);
extern Axel_state_code init_graphic_vbe(Vbe_info_block const* const, Vbe_mode_info_block const* const);
