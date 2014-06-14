/************************************************************
 * File: include/graphic.h
 * Description: graphic code header.
 ************************************************************/

#ifndef _GRAPHIC_H_
#define _GRAPHIC_H_


#include <flag.h>
#include <rgb8.h>
#include <state_code.h>
#include <vbe.h>

#ifdef GRAPHIC_MODE
#include <point.h>
#include <drawable.h>
#include <stdint.h>
#include <multiboot.h>
#endif

extern Axel_state_code init_graphic(Multiboot_info const * const);
extern void clean_screen(RGB8 const* const);

extern int putchar(int);

#ifdef GRAPHIC_MODE
/* functions in graphic mode only. */
extern uint32_t get_max_x_resolution(void);
extern uint32_t get_max_y_resolution(void);
extern void fill_rectangle(Point2d const* const, Point2d const* const, RGB8 const* const);
extern void draw_bitmap(Drawable_bitmap const* const, Point2d const* const);
extern void draw_nmulti_bitmap(Drawable_multi_bitmap const* const, Point2d const* const, uint32_t);
extern void draw_multi_bitmap(Drawable_multi_bitmap const* const, Point2d const* const);
extern void put_ascii_font(char const, Point2d const* const);
extern void puts_ascii_font(char const*, Point2d const* const);
extern void test_draw(RGB8 const* const);
extern void draw_mouse_cursor(Point2d const* const);
#endif


#endif
