/************************************************************
 * File: graphic.c
 * Description: some output functions for vbe Graphic.
 ************************************************************/


#include <graphic_vbe.h>
#include <state_code.h>
#include <vbe.h>

#define MAX_COLOR_RANGE 255

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


static Vbe_info_block const* info;
static Vbe_mode_info_block const* m_info;

static volatile uint8_t* vram;
static uint8_t byte_per_pixel;
static uint32_t vram_size;
static uint32_t max_x_resolution, max_y_resolution, max_xy_resolution;
static Color_bit_info bit_info;
static void (*set_vram) (uint32_t const, uint32_t const, RGB8 const * const);
static void set_vram8880(uint32_t const, uint32_t const, RGB8 const * const);
static void set_vram8888(uint32_t const, uint32_t const, RGB8 const * const);
static void set_vram5650(uint32_t const, uint32_t const, RGB8 const * const);
static inline uint32_t get_vram_index(uint32_t const, uint32_t const);


Axel_state_code init_graphic_vbe(Vbe_info_block const* const in, Vbe_mode_info_block const* const mi) {
    if (m_info->phys_base_ptr == 0) {
        return AXEL_FAILED;
    }

    info = in;
    m_info = mi;

    // XXX:
    vram = (uint8_t*)((intptr_t)m_info->phys_base_ptr);

    max_x_resolution = m_info->x_resolution;
    max_y_resolution = m_info->y_resolution;
    max_xy_resolution = max_x_resolution * max_y_resolution;

    byte_per_pixel = (m_info->bits_per_pixel / 8);
    vram_size = max_xy_resolution * byte_per_pixel;

    // store bit infomation.
    bit_info.r_size = m_info->red_mask_size;
    bit_info.g_size = m_info->green_mask_size;
    bit_info.b_size = m_info->blue_mask_size;
    bit_info.rsvd_size = m_info->red_mask_size;
    bit_info.r_pos = m_info->red_field_position;
    bit_info.g_pos = m_info->green_field_position;
    bit_info.b_pos = m_info->blue_field_position;
    bit_info.rsvd_pos = m_info->rsvd_field_position;

    switch (bit_info.serialised_size) {
        case 0x08080800:
            /* 8:8:8:0 */
            set_vram = set_vram8880;
            break;
        case 0x08080808:
            /* 8:8:8:8 */
            set_vram = set_vram8888;
            break;
        case 0x05060500:
            /* 5:6:5:0 */
            set_vram = set_vram5650;
            break;
        default:
            return AXEL_FAILED;
    }

    return AXEL_SUCCESS;
}


void clean_screen_vbe(RGB8 const * const c) {
    for (uint32_t i = 0; i < max_y_resolution; ++i) {
        for (uint32_t j = 0; j < max_x_resolution; ++j) {
            set_vram(j, i, c);
        }
    }

    *(uint32_t*)0xA0010 = max_x_resolution;
    *(uint32_t*)0xA0020 = max_y_resolution;
    *(uint32_t*)0xA0030 = max_xy_resolution;
    *(uint32_t*)0xA0040 = byte_per_pixel;

    /* *(volatile uint32_t*)(vram + (max_xy_resolution * 4)) = ((uint32_t)c->r << bit_info.r_pos) | ((uint32_t)c->g << bit_info.g_pos) | ((uint32_t)c->b << bit_info.b_pos) | ((uint32_t)c->rsvd << bit_info.rsvd_pos); */
    /* *(volatile uint32_t*)(vram + ((max_x_resolution + max_x_resolution * max_y_resolution) * 4)) = 0xFFFFFFFF; */

    /* for (uint32_t i = 0; i < vram_size; i++) { */
        /* vram[i] = 0xFF; */
    /* } */
}


static inline uint32_t get_vram_index(uint32_t const x, uint32_t const y) {
    return (x + max_x_resolution * y) * byte_per_pixel;
}

static void set_vram8880(uint32_t const x, uint32_t const y, RGB8 const * const c) {
    uint32_t const base = get_vram_index(x, y);

    vram[base] = c->r;
    vram[base + 1] = c->g;
    vram[base + 2] = c->b;
}


static void set_vram8888(uint32_t const x, uint32_t const y, RGB8 const * const c) {

    *(volatile uint32_t*)(vram + get_vram_index(x, y)) = ((uint32_t)c->r << bit_info.r_pos) | ((uint32_t)c->g << bit_info.g_pos) | ((uint32_t)c->b << bit_info.b_pos) | ((uint32_t)c->rsvd << bit_info.rsvd_pos);

    /* vram[base    ] = c->r; */
    /* vram[base + 1] = c->g; */
    /* vram[base + 2] = c->b; */
    /* vram[base + 3] = c->rsvd; */
}


static void set_vram5650(uint32_t const x, uint32_t const y, RGB8 const * const c) {
    uint32_t const base = get_vram_index(x, y);

    /* convert to 5, 6 and 5 bit */
    vram[base] = c->r << 3;
    vram[base + 1] = c->g << 2;
    vram[base + 2] = c->b << 3;
}
