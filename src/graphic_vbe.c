/**
 * @file graphic_vbe.c
 * @brief some output functions for vbe Graphic.
 * @author mopp
 * @version 0.1
 * @date 2014-10-13
 */


#include <graphic_vbe.h>
#include <asm_functions.h>
#include <macros.h>
#include <point.h>
#include <font.h>
#include <kernel.h>
#include <vbe.h>
#include <ps2.h>
#include <paging.h>


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

static volatile intptr_t vram;
static int8_t byte_per_pixel;
static int32_t vram_size;
static int32_t max_x_resolution, max_y_resolution, max_xy_resolution;
static Color_bit_info bit_info;
static Point2d pos;


void (*set_vram)(int32_t const, int32_t const, RGB8 const* const);
void (*set_vram_area)(Point2d const*, Point2d const*, RGB8 const*);
static void set_vram8880(int32_t const, int32_t const, RGB8 const* const);
static void set_vram8888(int32_t const, int32_t const, RGB8 const* const);
static void set_vram5650(int32_t const, int32_t const, RGB8 const* const);
static void set_vram_area8888(Point2d const*, Point2d const*, RGB8 const*);



Axel_state_code init_graphic_vbe(Multiboot_info const * const mb_info) {
    Vbe_mode_info_block const* const m_info = (Vbe_mode_info_block*)(uintptr_t)mb_info->vbe_mode_info;
    if (m_info->phys_base_ptr == 0) {
        /* failsafe */
        max_x_resolution = 320;
        max_y_resolution = 200;
        max_xy_resolution = max_x_resolution * max_y_resolution;
        byte_per_pixel = 1;
        vram_size = max_xy_resolution * byte_per_pixel;
        vram = (intptr_t)0xc00A0000;
        bit_info.serialised_size = 0x08080800;
        bit_info.r_pos = 16;
        bit_info.g_pos = 8;
        bit_info.b_pos = 0;
        set_vram = set_vram8880;
        set_point2d(&pos, 0, 0);
        return AXEL_FAILED;
    }

    max_x_resolution  = m_info->x_resolution;
    max_y_resolution  = m_info->y_resolution;
    max_xy_resolution = max_x_resolution * max_y_resolution;
    byte_per_pixel    = (m_info->bits_per_pixel / 8);
    vram_size         = max_xy_resolution * byte_per_pixel;
    vram              = (intptr_t)m_info->phys_base_ptr;

    /*
     * set vram virtual memory area.
     * bochs 0xe0000000
     * qemu 0xfd000000
     */
    map_page_same_area(get_kernel_pdt(), PDE_FLAGS_KERNEL, PTE_FLAGS_KERNEL, (uintptr_t)vram, (uintptr_t)vram + (uintptr_t)vram_size);

    /* store bit infomation. */
    bit_info.r_size    = m_info->red_mask_size;
    bit_info.g_size    = m_info->green_mask_size;
    bit_info.b_size    = m_info->blue_mask_size;
    bit_info.rsvd_size = m_info->red_mask_size;
    bit_info.r_pos     = m_info->red_field_position;
    bit_info.g_pos     = m_info->green_field_position;
    bit_info.b_pos     = m_info->blue_field_position;
    bit_info.rsvd_pos  = m_info->rsvd_field_position;

    /* In order to determine function, check display color mode. */
    switch (bit_info.serialised_size) {
        case 0x08080800:
            /* 8:8:8:0 */
            set_vram = set_vram8880;
            break;
        case 0x08080808:
            /* 8:8:8:8 */
            set_vram = set_vram8888;
            set_vram_area = set_vram_area8888;
            break;
        case 0x05060500:
            /* 5:6:5:0 */
            set_vram = set_vram5650;
            break;
        default:
            return AXEL_FAILED;
    }

    set_point2d(&pos, 0, 0);

    return AXEL_SUCCESS;
}


void clean_screen_vbe(RGB8 const* const c) {
    for (int32_t i = 0; i < max_y_resolution; ++i) {
        for (int32_t j = 0; j < max_x_resolution; ++j) {
            set_vram(j, i, c);
        }
    }
}


int32_t get_max_x_resolution(void) {
    return max_x_resolution;
}


int32_t get_max_y_resolution(void) {
    return max_y_resolution;
}


void fill_rectangle(Point2d const* const p0, Point2d const* const p1, RGB8 const* const c) {
    for (int32_t y = p0->y; y <= p1->y; ++y) {
        for (int32_t x = p0->x; x <= p1->x; ++x) {
            set_vram(x, y, c);
        }
    }
}


void draw_bitmap(Drawable_bitmap const* const dbmp, Point2d const* const p) {
    for (int32_t i = 0; i < dbmp->height; i++) {
        for (int32_t j = 0; j < dbmp->width; j++) {
            /* if bit is 1, draw color */
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


void draw_mouse_cursor(void) {
    if (axel_s.mouse->is_pos_update == false) {
        return;
    }
    axel_s.mouse->is_pos_update = false;

    int32_t const max_x = get_max_x_resolution() - mouse_cursor->width;
    int32_t const max_y = get_max_y_resolution() - mouse_cursor->height;
    if (max_x <= axel_s.mouse->pos.x) {
        axel_s.mouse->pos.x = max_x;
    }
    if (max_y <= axel_s.mouse->pos.y) {
        axel_s.mouse->pos.y = max_y;
    }
    if (axel_s.mouse->pos.y < 0) {
        axel_s.mouse->pos.y = 0;
    }
    if (axel_s.mouse->pos.x < 0) {
        axel_s.mouse->pos.x = 0;
    }

    for (uint8_t i = 0; i < 2; i++) {
        draw_bitmap(mouse_cursor + i, &axel_s.mouse->pos);
    }
}


void draw_multi_bitmap(Drawable_multi_bitmap const* const dmbmp, Point2d const* const p) {
    static Drawable_bitmap tb;
    static Point2d tp;
    tb.color = dmbmp->color;
    tb.height = dmbmp->height;
    tb.width = dmbmp->width;

    for (int32_t i = 0; i < dmbmp->size; i++) {
        tb.data = dmbmp->data[i];
        draw_bitmap(&tb, set_point2d(&tp, p->x + (i * dmbmp->width), p->y));
    }
}


int put_ascii_font(int const c, Point2d const* const p) {
    draw_nmulti_bitmap(&mplus_fonts, p, (uint32_t)c);
    return c;
}


void puts_ascii_font(char const *c, Point2d const* const p) {
    static Point2d tp;
    int32_t cnt = 0;

    while (*c!= '\0') {
        put_ascii_font(*c, set_point2d(&tp, p->x + (cnt * mplus_fonts.width), p->y));
        ++c;
        ++cnt;
    }
}


int putchar_vbe(int c) {
    int32_t const fwidth = mplus_fonts.width;
    int32_t const fheight = mplus_fonts.height;

    if (c == '\n' || c == '\r') {
        set_point2d(&pos, 0, pos.y + fheight);
        return c;
    }

    if (max_x_resolution <= (pos.x + fwidth)) {
        set_point2d(&pos, 0, pos.y + fheight);
    }

    if (max_y_resolution <= (pos.y + fheight)) {
        set_point2d(&pos, 0, 0);
        /* static const RGB8 cc = {.r = 0x3A, .g = 0x6E, .b = 0xA5}; */
        /* clean_screen_vbe(&cc); */
    }

    put_ascii_font(c, &pos);
    add_point2d(&pos, fwidth, 0);

    return c;
}


/*
 * As a result of acceleration, These are defined by macro.
 * NOTE: You must NOT use argument 'c' with "++"/"--"
 */
#define get_vram_index(x, y) (((x)+max_x_resolution * (y)) * byte_per_pixel)
#define shift_red(r)     ((uint32_t)(r) << bit_info.r_pos)
#define shift_green(g)   ((uint32_t)(g) << bit_info.g_pos)
#define shift_blue(b)    ((uint32_t)(b) << bit_info.b_pos)
#define shift_rsvd(rsvd) ((uint32_t)(rsvd) << bit_info.rsvd_pos)
#define get_color8888(c) (shift_red((c)->r) | shift_green((c)->g) | shift_blue((c)->b) | shift_rsvd((c)->rsvd))
#define get_color8880(c) ((shift_red((c)->r) | shift_green((c)->g) | shift_blue((c)->b)) << 8)
#define get_color5650(c) ((shift_red((c)->r >> 3) | shift_green((c)->g >> 2) | shift_blue((c)->b >> 3)) << 8)


static inline void set_vram8880(int32_t const x, int32_t const y, RGB8 const* const c) {
    volatile uint32_t* target = (volatile uint32_t*)(vram + get_vram_index(x, y));

    /* clear */
    *target &= 0x000000FF;

    *target |= get_color8880(c);
}


static inline void set_vram8888(int32_t const x, int32_t const y, RGB8 const* const c) {
    *(volatile uint32_t*)(vram + get_vram_index(x, y)) = get_color8888(c);
}


static inline void set_vram5650(int32_t const x, int32_t const y, RGB8 const* const c) {
    volatile uint32_t* target = (volatile uint32_t*)(vram + get_vram_index(x, y));

    /* clear */
    *target &= 0x000000FF;

    /* convert 8 bit to 5, 6 and 5 bit */
    *target |= get_color5650(c);
}


static inline void set_vram_area8888(Point2d const* begin, Point2d const* end, RGB8 const* colors) {
    intptr_t const begin_x = begin->x * byte_per_pixel;
    intptr_t const begin_y = (begin->y * max_x_resolution) * byte_per_pixel;
    intptr_t const end_x   = end->x * byte_per_pixel;
    intptr_t const end_y   = (end->y * max_x_resolution) * byte_per_pixel;
    intptr_t const dy      = max_x_resolution * byte_per_pixel;
    RGB8 const* base_c     = &colors[begin->x + (begin->y * max_x_resolution)];

    intptr_t vy_e = vram + end_y;
    for (register intptr_t vy = vram + begin_y; vy < vy_e; vy += dy) {
        register RGB8 const* c = base_c;
        intptr_t const ve = (vy + end_x);
        for (register intptr_t v = (vy + begin_x); v < ve; v += byte_per_pixel, c++) {
            *((volatile uint32_t*)v) = get_color8888(c);
        }
        base_c += max_x_resolution;
    }
}
