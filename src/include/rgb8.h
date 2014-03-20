/****************************************************************************************
 * @file include/rgb8.h
 * @brief rgb8 header.
 * @author mopp
 * @version 0.1
 * @date 2014-03-13
 ****************************************************************************************/

#ifndef RGB8_H
#define RGB8_H


#include <stdint.h>

/* This represents Red, Green, Blue and reserved color value(0 - 255). */
struct RGB8 {
    uint8_t r, g, b, rsvd;
};
typedef struct RGB8 RGB8;


static inline RGB8* set_rgb_by_color(RGB8* const rgb, uint32_t color) {
    rgb->r = 0xFF & (color >> 16);
    rgb->g = 0xFF & (color >> 8);
    rgb->b = (0xFF & color);
    return rgb;
}


static inline RGB8 convert_color2RGB8(uint32_t color) {
    return (RGB8) {r : (0xFF & color >> 16), g : (0xFF & (color >> 8)), b : (0xFF & color), rsvd : 0};
}


static inline RGB8 convert_each_color2RGB8(uint8_t const cr, uint8_t const cg, uint8_t const cb) {
    return (RGB8) {r : cr, g : cg, b : cb, rsvd : 0};
}


#endif
