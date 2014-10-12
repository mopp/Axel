/**
 * @file rgb8.c
 * @brief rgb8 implementation.
 * @author mopp
 * @version 0.1
 * @date 2014-10-13
 */


#include <rgb8.h>
#include <macros.h>


RGB8* set_rgb_by_color(RGB8* const rgb, uint32_t color) {
    rgb->r = ECAST_UINT8(color >> 16);
    rgb->g = ECAST_UINT8(color >> 8);
    rgb->b = ECAST_UINT8(color);
    return rgb;
}


RGB8 convert_color2RGB8(uint32_t color) {
    return (RGB8){
        .r = ECAST_UINT8(color >> 16),
        .g = ECAST_UINT8(color >> 8),
        .b = ECAST_UINT8(color),
        .rsvd = 0
    };
}


RGB8 convert_each_color2RGB8(uint8_t const cr, uint8_t const cg, uint8_t const cb) {
    return (RGB8){
        .r = cr,
        .g = cg,
        .b = cb,
        .rsvd = 0
    };
}
