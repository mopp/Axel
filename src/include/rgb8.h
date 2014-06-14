/****************************************************************************************
 * @file include/rgb8.h
 * @brief rgb8 header.
 * @author mopp
 * @version 0.1
 * @date 2014-03-13
 ****************************************************************************************/

#ifndef _RGB8_H_
#define _RGB8_H_


#include <stdint.h>
#include <macros.h>

/* This represents Red, Green, Blue and reserved color value(0 - 255). */
union RGB8 {
    struct {
        uint8_t r, g, b, rsvd;
    };
    uint32_t bit_expr;
};
typedef union RGB8 RGB8;


static inline RGB8* set_rgb_by_color(RGB8* const rgb, uint32_t color) {
    rgb->r = ECAST_UINT8(color >> 16);
    rgb->g = ECAST_UINT8(color >> 8);
    rgb->b = ECAST_UINT8(color);
    return rgb;
}


static inline RGB8 convert_color2RGB8(uint32_t color) {
    return (RGB8) {.r = ECAST_UINT8(color >> 16), .g = ECAST_UINT8(color >> 8), .b = ECAST_UINT8(color), .rsvd = 0};
}


static inline RGB8 convert_each_color2RGB8(uint8_t const cr, uint8_t const cg, uint8_t const cb) {
    return (RGB8) {.r = cr, .g = cg, .b = cb, .rsvd = 0};
}


#endif
