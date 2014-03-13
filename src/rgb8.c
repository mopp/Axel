/****************************************************************************************
 * @file rgb8.h
 * @brief rgb8 structure shows red, green and blue color and reserved area.
 *          And this contains utility functions for rgb8.
 * @author mopp
 * @version 0.1
 * @date 2014-03-13
 ****************************************************************************************/

#include <rgb8.h>

RGB8 convert_RGB8(uint8_t const cr, uint8_t const cg, uint8_t const cb) {
    return (RGB8){ r:cr, g:cg, b:cb, rsvd:0 };
}
