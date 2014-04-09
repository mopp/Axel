/************************************************************
 * File: graphic.c
 * Description: some output functions for vbe Graphic.
 ************************************************************/

#include <graphic_vbe.h>
#include <macros.h>
#include <state_code.h>
#include <vbe.h>
#include <point.h>
#include <font.h>
#include <drawable.h>

/* This represents bit size and position infomation from VBE. */
struct Color_bit_info {
    union {
        struct {
            /* this shows each bit size. */
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

static inline uint32_t get_vram_index(uint32_t const, uint32_t const);
static inline uint32_t get_shift_red(uint8_t);
static inline uint32_t get_shift_green(uint8_t);
static inline uint32_t get_shift_blue(uint8_t);
static inline uint32_t get_shift_rsvd(uint8_t);
static void (*set_vram)(uint32_t const, uint32_t const, RGB8 const* const);
static inline void set_vram8880(uint32_t const, uint32_t const, RGB8 const* const);
static inline void set_vram8888(uint32_t const, uint32_t const, RGB8 const* const);
static inline void set_vram5650(uint32_t const, uint32_t const, RGB8 const* const);


Axel_state_code init_graphic_vbe(Vbe_info_block const* const in, Vbe_mode_info_block const* const mi) {
    if (m_info->phys_base_ptr == 0) {
        return AXEL_FAILED;
    }

    /* store to file local variable */
    info = in;
    m_info = mi;
    max_x_resolution = m_info->x_resolution;
    max_y_resolution = m_info->y_resolution;
    max_xy_resolution = max_x_resolution * max_y_resolution;
    byte_per_pixel = (m_info->bits_per_pixel / 8);
    vram_size = max_xy_resolution * byte_per_pixel;
    vram = (uint8_t*)((uintptr_t)m_info->phys_base_ptr);

    /* store bit infomation. */
    bit_info.r_size = m_info->red_mask_size;
    bit_info.g_size = m_info->green_mask_size;
    bit_info.b_size = m_info->blue_mask_size;
    bit_info.rsvd_size = m_info->red_mask_size;
    bit_info.r_pos = m_info->red_field_position;
    bit_info.g_pos = m_info->green_field_position;
    bit_info.b_pos = m_info->blue_field_position;
    bit_info.rsvd_pos = m_info->rsvd_field_position;

    /* In order to determine function, check display color mode. */
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


void clean_screen_vbe(RGB8 const* const c) {
    for (uint32_t i = 0; i < max_y_resolution; ++i) {
        for (uint32_t j = 0; j < max_x_resolution; ++j) {
            set_vram(j, i, c);
        }
    }
}


uint32_t get_max_x_resolution(void) {
    return max_x_resolution;
}


uint32_t get_max_y_resolution(void) {
    return max_y_resolution;
}


void fill_rectangle(Point2d const* const p0, Point2d const* const p1, RGB8 const* const c) {
    for (uint32_t y = p0->y; y <= p1->y; ++y) {
        for (uint32_t x = p0->x; x <= p1->x; ++x) {
            set_vram(x, y, c);
        }
    }
}


void draw_bitmap(Drawable_bitmap const* const dbmp, Point2d const* const p) {
    for (uint32_t i = 0; i < dbmp->height; i++) {
        for (uint32_t j = 0; j < dbmp->width; j++) {
            if (1 == (0x01 & (dbmp->data[i] >> (dbmp->width - j - 1)))) {
                set_vram(p->x + j, p->y + i, &dbmp->color);
            }
        }
    }
}


void draw_nmulti_bitmap(Drawable_multi_bitmap const* const dmbmp, Point2d const* const p, uint32_t n) {
    static Drawable_bitmap tb;
    tb.color = dmbmp->color;
    tb.height = dmbmp->height;
    tb.width = dmbmp->width;
    tb.data = dmbmp->data[n];

    draw_bitmap(&tb, p);
}


void draw_multi_bitmap(Drawable_multi_bitmap const* const dmbmp, Point2d const* const p) {
    static Drawable_bitmap tb;
    static Point2d tp;
    tb.color = dmbmp->color;
    tb.height = dmbmp->height;
    tb.width = dmbmp->width;

    for (uint8_t i = 0; i < dmbmp->size; i++) {
        tb.data = dmbmp->data[i];
        draw_bitmap(&tb, set_point2d(&tp, p->x + (i * dmbmp->width), p->y));
    }
}


void put_ascii_font(char const *c, Point2d const* const p) {
    static Point2d tp;
    uint32_t cnt = 0;

    while (*c!= '\0') {
        draw_nmulti_bitmap(&mplus_fonts, set_point2d(&tp, p->x + (cnt * mplus_fonts.width), p->y), (uint32_t)*c);
        ++c;
        ++cnt;
    }
}

#define font_color { r : 240, g : 96, b : 173, rsvd : 0 }

void test_draw(RGB8 const* const c) {
    /* TODO: move other place. */
    static uint32_t const font_mo[] = {
        0x0200, /* ......@.........*/
        0x0200, /* ......@.........*/
        0x0200, /* ......@.........*/
        0x1fc0, /* ...@@@@@@@@.....*/
        0x0200, /* ......@.........*/
        0x0200, /* ......@.........*/
        0x0460, /* .....@...@@.....*/
        0x3f80, /* ..@@@@@@@.......*/
        0x0400, /* .....@..........*/
        0x0404, /* .....@.......@..*/
        0x0404, /* .....@.......@..*/
        0x0404, /* .....@.......@..*/
        0x0208, /* ......@.....@...*/
        0x0110, /* .......@...@....*/
        0x00e0, /* ........@@@.....*/
        0x0000, /* ................*/
    };
    static uint32_t const font_pu[] = {
        0x000c,
        0x0212,
        0x01d2,
        0x004c,
        0x0080,
        0x0100,
        0x0100,
        0x0088,
        0x1044,
        0x1042,
        0x2021,
        0x2021,
        0x4020,
        0x0440,
        0x0380,
        0x0000,
    };
    static uint32_t const font_ri[] = {
        0x0020,
        0x0410,
        0x0410,
        0x0410,
        0x0810,
        0x0810,
        0x0810,
        0x0810,
        0x0a10,
        0x0410,
        0x0020,
        0x0020,
        0x0040,
        0x0080,
        0x0300,
        0x0000,
    };
    static uint32_t const font_n[] = {
        0x0080,
        0x0080,
        0x0100,
        0x0100,
        0x0200,
        0x0200,
        0x0200,
        0x0580,
        0x0640,
        0x0c40,
        0x0840,
        0x1041,
        0x1042,
        0x2024,
        0x2018,
        0x0000,
    };
    static uint32_t const font_o[] = {
        0x0000,
        0x03c0,
        0x0c30,
        0x1818,
        0x1008,
        0x2004,
        0x2004,
        0x2004,
        0x2004,
        0x2004,
        0x1008,
        0x1818,
        0x0c30,
        0x03c0,
        0x0000,
        0x0000,
    };
    static uint32_t const font_s[] = {
        0x0000,
        0x03d0,
        0x0430,
        0x0810,
        0x0800,
        0x0800,
        0x0400,
        0x03c0,
        0x0030,
        0x0008,
        0x1008,
        0x1808,
        0x1410,
        0x03e0,
        0x0000,
        0x0000,
    };

    static const Drawable_multi_bitmap tes = {
        height : 16,
        width : 16,
        size : 6,
        color : font_color,
        data : {
            font_mo,
            font_pu,
            font_ri,
            font_n,
            font_o,
            font_s,
        }
    };

    /* もぷりんOS */
    uint32_t x = (max_x_resolution / 2) - (16 * 3);
    draw_multi_bitmap(&tes, &make_point2d(x, max_y_resolution / 2 + 8));

    uint32_t y = 100;
    put_ascii_font(" __  __ ",                      &make_point2d(x, y += 13));
    put_ascii_font("|  \\/  |",                     &make_point2d(x, y += 13));
    put_ascii_font("| \\  / | ___  _ __  _ __",     &make_point2d(x, y += 13));
    put_ascii_font("| |\\/| |/ _ \\| '_ \\| '_ \\", &make_point2d(x, y += 13));
    put_ascii_font("| |  | | (_) | |_) | |_) |",    &make_point2d(x, y += 13));
    put_ascii_font("|_|  |_|\\___/| .__/| .__/",    &make_point2d(x, y += 13));
    put_ascii_font("             | |   | |",        &make_point2d(x, y += 13));
    put_ascii_font("             |_|   |_|",        &make_point2d(x, y += 13));
}


static inline uint32_t get_vram_index(uint32_t const x, uint32_t const y) {
    return (x + max_x_resolution * y) * byte_per_pixel;
}


static inline uint32_t get_shift_red(uint8_t r) {
    return (uint32_t)r << bit_info.r_pos;
}


static inline uint32_t get_shift_green(uint8_t g) {
    return (uint32_t)g << bit_info.g_pos;
}


static inline uint32_t get_shift_blue(uint8_t b) {
    return (uint32_t)b << bit_info.b_pos;
}


static inline uint32_t get_shift_rsvd(uint8_t rsvd) {
    return (uint32_t)rsvd << bit_info.rsvd_pos;
}


static inline void set_vram8880(uint32_t const x, uint32_t const y, RGB8 const* const c) {
    volatile uint32_t* target = (volatile uint32_t*)(vram + get_vram_index(x, y));

    /* clear */
    *target &= 0x000000FF;

    *target |= (get_shift_red(c->r) | get_shift_green(c->g) | get_shift_blue(c->b)) << 8;
}


static inline void set_vram8888(uint32_t const x, uint32_t const y, RGB8 const* const c) {
    *(volatile uint32_t*)(vram + get_vram_index(x, y)) = get_shift_red(c->r) | get_shift_green(c->g) | get_shift_blue(c->b) | get_shift_rsvd(c->rsvd);
}


static inline void set_vram5650(uint32_t const x, uint32_t const y, RGB8 const* const c) {
    volatile uint32_t* target = (volatile uint32_t*)(vram + get_vram_index(x, y));

    /* clear */
    *target &= 0x000000FF;

    /* convert 8 bit to 5, 6 and 5 bit */
    *target |= (get_shift_red(c->r >> 3) | get_shift_green(c->g >> 2) | get_shift_blue(c->b >> 3)) << 8;
}
