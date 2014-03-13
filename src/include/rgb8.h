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


extern RGB8 convert_RGB8(uint8_t const, uint8_t const, uint8_t const);


#endif
