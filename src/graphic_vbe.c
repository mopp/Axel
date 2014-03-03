/************************************************************
 * File: graphic.c
 * Description: some output functions for Normal Graphic.
 ************************************************************/


#include <graphic.h>
#include <vbe.h>
#include <state_code.h>

#define MAX_COLOR_RANGE 255

/* 型に依存しないようにマクロ. */
/* 引数に*cのようなものを渡すときのために括弧を付ける. */
#define gen_rgb(c) (((c).r << bit_info.r_pos) + ((c).g << bit_info.g_pos) + ((c).b << bit_info.b_pos) + ((c).rsvd << bit_info.rsvd_pos))


/* This represents Red, Green, Blue and reserved color value(0 - 255). */
struct RGB8 {
    uint8_t r, g, b, rsvd;
};
typedef struct RGB8 RGB8;


/* This represents bit size and position infomation from VBE. */
struct Color_bit_info {
    union {
        /* this shows each bit size. */
        struct {
            uint8_t r_size, g_size, b_size, rsvd_size;
        };
        uint32_t serialised_size;
    };
    /* this shows each bit position. */
    uint8_t r_pos, g_pos, b_pos, rsvd_pos;
};
typedef struct Color_bit_info Color_bit_info;


// TODO:fix name
/* static volatile char* vram; */
/* static int byte_per_pixel, vram_size; */
/* static uint32_t rgb_map[MAX_COLOR_RANGE * MAX_COLOR_RANGE * MAX_COLOR_RANGE]; */
/* static Vbe_info_block const* info; */
/* static Vbe_mode_info_block const* m_info; */
// RGB each mask size and bit position.
/* static Color_bit_info bit_info; */

// accesser for rgb_map.
/* static inline uint32_t get_rgb_map(uint8_t const, uint8_t const, uint8_t const); */
/* static inline uint32_t set_rgb_map(uint8_t const, uint8_t const, uint8_t const, uint32_t); */


/* Axel_state_code init_graphic_todo(Vbe_info_block const* const in, Vbe_mode_info_block const* const mi) { */
    /* if (m_info->phys_base_ptr == 0) { */
        /* return AXEL_FAILED; */
    /* } */

    /* info = in; */
    /* m_info = mi; */

    /* [> byte_per_pixel = (m_info->bits_per_pixel / 8); <] */
    /* [> vram_size = m_info->x_resolution * m_info->y_resolution * byte_per_pixel; <] */

    /* bit_info.r_size = m_info->red_mask_size; */
    /* bit_info.g_size = m_info->green_mask_size; */
    /* bit_info.b_size = m_info->blue_mask_size; */
    /* bit_info.rsvd_size = m_info->red_mask_size; */
    /* bit_info.r_pos = m_info->red_field_position; */
    /* bit_info.g_pos = m_info->green_field_position; */
    /* bit_info.b_pos = m_info->blue_field_position; */
    /* bit_info.rsvd_pos = m_info->rsvd_field_position; */

    /* if (bit_info.serialised_size == 0x08080808) { */
        /* // 8:8:8 */

        /* // MAX_COLOR_RANGE is 255, it also equals value of max 8-bit. */
        /* int const dr = 1 << bit_info.r_pos; */
        /* int const dg = 1 << bit_info.g_pos; */
        /* int const db = 1 << bit_info.b_pos; */

        /* for (int r = 0; r < MAX_COLOR_RANGE; r += dr) { */
            /* for (int g = 0; g < MAX_COLOR_RANGE; g += dg) { */
                /* for (int b = 0; b < MAX_COLOR_RANGE; b += db) { */
                    /* set_rgb_map(r, g, b, (r + g + b)); */
                /* } */
            /* } */
        /* } */
    /* } else if (bit_info.serialised_size == 0x05060500) { */
        /* // 5:6:5 */
    /* } else { */
        /* return AXEL_FAILED; */
    /* } */

    /* return AXEL_SUCCESS; */
/* } */


/* uint32_t get_rgb(uint8_t const r, uint8_t const g, uint8_t const b) { */
    /* return get_rgb_map(r, g, b); */
/* } */


void clean_screen_g(uint32_t const background_rgb) {
    /* for (int i = 0; i < vram_size; i++) { */
        /* vram[i] = background_rgb; */
    /* } */
}


static uint8_t* set_vram5650(uint8_t* const v, RGB8* color) {
    /* uint16_t c = gen_rgb(*color); */
    /* uint8_t t[] = {c & 0x00FF, (c & 0xFF00) >> 8}; */

    for (int i = 0; i < 2; ++i) {
        /* v[i] = t[i]; */
    }

    return v;
}


static uint8_t* set_vram8880(uint8_t* const v, RGB8 *color) {
    uint8_t const t[] = {color->r, color->g, color->b};

    for (int i = 0; i < 3; i++) {
        v[i] = t[i];
    }

    return v;
}


static uint8_t* set_vram8888(uint8_t* const v, RGB8* color) {
    uint8_t const t[] = {color->r, color->g, color->b, 0};

    for (int i = 0; i < sizeof(t) / sizeof(t[0]); i++) {
        v[i] = t[i];
    }

    return v;
}


/* static inline uint32_t get_rgb_map(uint8_t const r, uint8_t const g, uint8_t const b) { */
    /* return rgb_map[r * MAX_COLOR_RANGE + g * MAX_COLOR_RANGE + b * MAX_COLOR_RANGE]; */
/* } */


static inline uint32_t set_rgb_map(uint8_t const r, uint8_t const g, uint8_t const b, uint32_t v) {
    /* (r << 8 - 1) */
    /* rgb_map[r * MAX_COLOR_RANGE + g * MAX_COLOR_RANGE + b * MAX_COLOR_RANGE] = v; */
    return v;
}
