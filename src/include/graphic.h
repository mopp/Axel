/************************************************************
 * File: include/graphic.h
 * Description: graphic code header.
 ************************************************************/

#ifndef GRAPHIC_H
#define GRAPHIC_H


#include <flag.h>
#include <rgb8.h>
#include <state_code.h>
#include <vbe.h>
#include <point.h>
#include <stdint.h>

extern Axel_state_code init_graphic(Vbe_info_block const* const, Vbe_mode_info_block const* const);
extern void clean_screen(RGB8 const* const);

extern int putchar(int);
extern const char* puts(const char*);


#ifdef GRAPHIC_MODE
/* functions in graphic mode only. */
extern void fill_rectangle(Point2d const * const, Point2d const * const, RGB8 const * const);
extern uint32_t get_max_x_resolution(void);
extern uint32_t get_max_y_resolution(void);
#endif

#ifdef TEXT_MODE
    // TODO: define color presets
#else
    // TODO: define color presets
#endif


#endif
