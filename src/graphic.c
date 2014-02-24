/************************************************************
 * File: graphic.c
 * Description: some output functions for Normal Graphic.
 ************************************************************/


#include <graphic.h>
#include <vbe.h>
#include <state_code.h>

#define MAX_COLOR_RANGE 255

/* RGB8 represents Red, Green, Blue and reserved parameter. */
// TODO: 色情報とビット情報は別の構造体にわける
struct RGB8 {
    union {
        // this shows each color value(0 - 255).
        struct {
            uint8_t r, g, b, rsvd;
        };
        // this shows each bit size.
        struct {
            uint8_t r_size, g_size, b_size, rsvd_size;
        };
        // 比較するときにそれぞれif文を書くのは手間なのでまとめて
        uint32_t all_size;
    };
    uint8_t r_pos, g_pos, b_pos, rsvd_pos;
};
typedef struct RGB8 RGB8;

static char* vram;
static int byte_per_pixel, vram_size;
static uint32_t rgb_map[MAX_COLOR_RANGE * MAX_COLOR_RANGE * MAX_COLOR_RANGE];
static Vbe_info_block const* info;
static Vbe_mode_info_block const* m_info;
// RGB each mask size and bit position.
static RGB8 rgb8;

// accesser for rgb_map.
static inline uint32_t get_rgb_map(uint8_t const, uint8_t const, uint8_t const);
static inline uint32_t set_rgb_map(uint8_t const, uint8_t const, uint8_t const, uint32_t);

Axel_state_code init_graphic(Vbe_info_block const* const in, Vbe_mode_info_block const* const mi) {
    if (m_info->phys_base_ptr == 0) {
        return AXEL_FAILED;
    }

    info = in;
    m_info = mi;

    byte_per_pixel = (m_info->bits_per_pixel / 8);
    vram_size = m_info->x_resolution * m_info->y_resolution * byte_per_pixel;

    rgb8.r_size = m_info->red_mask_size;
    rgb8.g_size = m_info->green_mask_size;
    rgb8.b_size = m_info->blue_mask_size;
    rgb8.rsvd_size = m_info->red_mask_size;
    rgb8.r_pos = m_info->red_field_position;
    rgb8.g_pos = m_info->green_field_position;
    rgb8.b_pos = m_info->blue_field_position;
    rgb8.rsvd_pos = m_info->rsvd_field_position;

    if (rgb8.all_size == 0x08080808) {
        // 8:8:8
        // MAX_COLOR_RANGE is 255, it also equals value of max 8-bit.
        for (int r = 0; r < MAX_COLOR_RANGE; ++r) {
            for (int g = 0; g < MAX_COLOR_RANGE; ++g) {
                for (int b = 0; b < MAX_COLOR_RANGE; ++b) {
                    set_rgb_map(r, g, b, ((r << rgb8.r_pos) + (g << rgb8.g_pos) + (b << rgb8.b_pos) + (0 << rgb8.rsvd_pos)));
                }
            }
        }
    } else if (rgb8.all_size == 0x05060500) {
        // 5:6:5
    } else {
        return AXEL_FAILED;
    }

    return AXEL_SUCCESS;
}

// 型に依存しないようにマクロ.
// 引数に*cのようなものを渡すときのために括弧を付ける.
#define gen_rgb(c) (((c).r << (c).r_pos) + ((c).g << (c).g_pos) + ((c).b << (c).b_pos) + ((c).rsvd << (c).rsvd_pos))

uint8_t* set_vram5650(uint8_t* const v, RGB8* color) {
    uint16_t c = gen_rgb(*color);
    uint8_t t[] = { c & 0x00FF, (c & 0xFF00) >> 8 };

    for (int i = 0; i < 2; ++i) {
        v[i] = t[i];
    }

    return v;
}


uint8_t* set_vram8880(uint8_t* const v, RGB8* color) {
    uint8_t t[] = { color->r, color->g, color->b };
    for (int i = 0; i < 3; i++) {
        v[i] = t[i];
    }

    return v;
}


uint32_t get_rgb(uint8_t const r, uint8_t const g, uint8_t const b) {
    return get_rgb_map(r, g, b);
}


void clean_screen_g(uint32_t const background_rgb) {
    for (int i = 0; i < vram_size; i++) {
        vram[i] = background_rgb;
    }
}


static inline uint32_t get_rgb_map(uint8_t const r, uint8_t const g, uint8_t const b) {
    return rgb_map[r * MAX_COLOR_RANGE + g * MAX_COLOR_RANGE + b * MAX_COLOR_RANGE];
}


static inline uint32_t set_rgb_map(uint8_t const r, uint8_t const g, uint8_t const b, uint32_t v) {
    rgb_map[r * MAX_COLOR_RANGE + g * MAX_COLOR_RANGE + b * MAX_COLOR_RANGE] = v;
    return v;
}

