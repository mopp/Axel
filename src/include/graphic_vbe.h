/************************************************************
 * File: include/graphic_vbe.h
 * Description: Graphic Header for VBE2.0+.
 ************************************************************/

#ifndef GRAPHIC_VBE_H
#define GRAPHIC_VBE_H


#include <rgb8.h>
#include <state_code.h>
#include <stdint.h>
#include <vbe.h>
#include <point.h>


enum Drawable_constants {
    MAX_HEIGHT_SIZE = 16,
    MAX_WIDTH_SIZE = 16,
};


struct Drawable_bitmap {
    uint32_t height;
    uint32_t width;
    RGB8 color;
    uint32_t const* data;
};
typedef struct Drawable_bitmap Drawable_bitmap;


struct Drawable_multi_bitmap {
    uint32_t height;
    uint32_t width;
    uint32_t size;
    RGB8 color;
    uint32_t const* data[];
};
typedef struct Drawable_multi_bitmap Drawable_multi_bitmap;


extern void clean_screen_vbe(RGB8 const* const);
extern Axel_state_code init_graphic_vbe(Vbe_info_block const* const, Vbe_mode_info_block const* const);
extern void draw_bitmap(Drawable_bitmap const* const, Point2d const* const);


#endif
