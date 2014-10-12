/**
 * @file include/rgb8.h
 * @brief rgb8 header.
 * @author mopp
 * @version 0.1
 * @date 2014-10-13
 */


#ifndef _RGB8_H_
#define _RGB8_H_



#include <stdint.h>


/* This represents Red, Green, Blue and reserved color value(0 - 255). */
union RGB8 {
    struct {
        uint8_t r, g, b, rsvd;
    };
    uint32_t bit_expr;
};
typedef union RGB8 RGB8;
_Static_assert(sizeof(RGB8) == 4, "Size of RGB8 is NOT 4 byte");


extern RGB8* set_rgb_by_color(RGB8* const , uint32_t);
extern RGB8 convert_color2RGB8(uint32_t);
extern RGB8 convert_each_color2RGB8(uint8_t const , uint8_t const , uint8_t const);



#endif
