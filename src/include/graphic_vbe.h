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
#include <drawable.h>
#include <point.h>


extern void clean_screen_vbe(RGB8 const* const);
extern Axel_state_code init_graphic_vbe(Multiboot_info const * const);
extern int32_t get_max_x_resolution(void);
extern int32_t get_max_y_resolution(void);
extern void fill_rectangle(Point2d const* const, Point2d const* const, RGB8 const* const);
extern void draw_bitmap(Drawable_bitmap const* const , Point2d const* const);
extern int putchar_vbe(int);
extern void (*set_vram)(int32_t const, int32_t const, RGB8 const* const);
extern void (*set_vram_area)(Point2d const*, Point2d const*, RGB8 const*);



#endif
