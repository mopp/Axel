/**
 * @file window.c
 * @brief implement for high GUI.
 * @author mopp
 * @version 0.1
 * @date 2014-06-17
 */


#include <window.h>
#include <paging.h>
#include <graphic.h>
#include <atype.h>
#include <state_code.h>
#include <string.h>


struct window {
    /* 800 * 600 * 4 Byte(sizeof(RGB8)) = 1875KB = 1.8MB*/
    RGB8* buf;
    Point2d pos;
    Point2d size;
    union {
        struct {
            uint8_t lock : 1;
            uint8_t dirty : 1;
            uint8_t enable : 1;
            uint8_t reserved : 5;
        };
        uint8_t flags;
    };
};


struct window_manager {
    List win_list;
};
typedef struct window_manager Window_manager;


static Window_manager* win_man;
static Point2d display_size;
static uint8_t* map;
static RGB8* display_buffer;
enum {
    COVER_MASK_BASE_SIZE = 8, /* mask is 8pixel/1bit */
};



static inline void calibrate(Point2d* p) {
    if (p->x < 0) {
        p->x = 0;
    }

    if (p->y < 0) {
        p->y = 0;
    }

    if (display_size.x < p->x) {
        p->x = display_size.x;
    }

    if (display_size.y < p->y) {
        p->y = display_size.y;
    }
}


Window* alloc_window(Point2d const* pos, Point2d const* size, uint8_t level) {
    Window w = {.lock = false, .dirty = true, .enable = true, .reserved = 0};

    w.pos.x = pos->x;
    w.pos.y = pos->y;
    w.size.x = size->x;
    w.size.y = size->y;

    calibrate(&w.pos);
    calibrate(&w.size);
    if (w.size.x < 32) {
        w.size.x = 32;
    }
    if (w.size.y < 32) {
        w.size.y = 32;
    }


    w.buf = vmalloc(sizeof(RGB8) * (size_t)w.size.x * (size_t)w.size.y);
    if (w.buf == NULL) {
        return NULL;
    }

    List* l = list_insert_data_last(&win_man->win_list, &w);
    if (l == NULL) {
        return NULL;
    }

    return win_man->win_list.node->prev->data;
}


Window* alloc_drawn_window(Point2d const* pos, Drawable_bitmap const * dw, size_t len) {

    Window* w = alloc_window(pos, &make_point2d(dw->width, dw->width), 0);
    if (w == NULL) {
        return NULL;
    }

    for (int k = 0; k < len; k++) {
        for (int32_t i = 0; i < dw[k].height; i++) {
            for (int32_t j = 0; j < dw[k].width; j++) {
                /* if bit is 1, draw color */
                if (1 == (0x01 & (dw[k].data[i] >> (dw[k].width - j - 1)))) {
                    /* set_vram(p->x + j, p->y + i, &dbmp->color); */
                    w->buf[j + w->size.y * i] = dw[k].color;
                }
            }
        }
    }

    return w;
}


Window* alloc_filled_window(Point2d const* pos, Point2d const* size, uint8_t level, RGB8 const* c) {
    Window* w = alloc_window(pos, size, level);
    if (w == NULL) {
        return NULL;
    }

    /* fill area. */
    for (int i = 0; i < w->size.y; i++) {
        for (int j = 0; j < w->size.x; j++) {
            w->buf[j + w->size.y * i] = *c;
        }
    }

    return w;
}


static void win_node_free(void* d) {
    if (d == NULL) {
        return;
    }

    Window* w = (Window*)d;

    vfree(w->buf);
    vfree(w);
}


static bool is_re_buffer;
static Point2d begin_p, end_p; /* check area. */
Axel_state_code init_window(void) {
    display_size.x = get_max_x_resolution();
    display_size.y = get_max_y_resolution();

    end_p = display_size;

    size_t const display_pixel = (size_t)(display_size.x * display_size.y);

    display_buffer = vmalloc(sizeof(RGB8) * display_pixel);
    if (display_buffer == NULL) {
        return AXEL_MEMORY_ALLOC_ERROR;
    }
    memset(display_buffer, 0, sizeof(RGB8) * display_pixel);

    map = vmalloc(display_pixel * sizeof(uint8_t));
    if (map == NULL) {
        return AXEL_MEMORY_ALLOC_ERROR;
    }
    memset(map, UINT8_MAX, display_pixel * sizeof(uint8_t));

    win_man = vmalloc(sizeof(Window_manager));
    if (NULL == win_man) {
        return AXEL_MEMORY_ALLOC_ERROR;
    }
    list_init(&win_man->win_list, sizeof(Window), win_node_free);

    return AXEL_SUCCESS;
}



void free_window(Window* w) {
    if (w == NULL) {
        return;
    }

    List_node* n = list_search_node(&win_man->win_list, w);
    if (n == NULL) {
        return;
    }

    list_delete_node(&win_man->win_list, n);
}


#include <stdio.h>
static bool update_win_for_each(void* d) {
    Window* w = d;

    /* if (w->dirty == false || w->enable == false) { */
    if (w->enable == false) {
        w->enable = true;
        return false;
    }

    /* NOTE: These point is NOT points of buffer and are display points.  */
    /* calculate check area points. */
    int32_t const ca_bx = (begin_p.x < 0) ? (0) : (begin_p.x);
    int32_t const ca_by = (begin_p.y < 0) ? (0) : (begin_p.y);
    int32_t const ca_ex = (display_size.x < end_p.x) ? (display_size.x) : (end_p.x);
    int32_t const ca_ey = (display_size.y < end_p.y) ? (display_size.y) : (end_p.y);
    /* calculate buf area points. */
    int32_t const ba_bx = w->pos.x;
    int32_t const ba_by = w->pos.y;
    int32_t const ba_ex = ba_bx + w->size.x;
    int32_t const ba_ey = ba_by + w->size.y;

    /* puts("--------------------------------------------------\n"); */
    /* printf("check - begin (%d, %d)\n", ca_bx, ca_by); */
    /* printf("check - end   (%d, %d)\n", ca_ex, ca_ey); */
    /* printf("buf   - begin (%d, %d)\n", ba_bx, ba_by); */
    /* printf("buf   - end   (%d, %d)\n", ba_ex, ba_ey); */

    /* checking Is this window included ? */
    if (!((ca_bx <= ba_bx || ba_bx <= ca_ex) || (ca_by <= ba_by || ba_by <= ca_ey))) {
        return false;
    }

    /* calculate drawing area */
    int32_t const da_bx = (ca_bx <= ba_bx) ? (ba_bx):(ca_bx);
    int32_t const da_by = (ca_by <= ba_by) ? (ba_by):(ca_by);
    int32_t const da_ex = (ca_ex <= ba_ex) ? (ca_ex):(ba_ex);
    int32_t const da_ey = (ca_ex <= ba_ey) ? (ca_ey):(ba_ey);
    /* printf("draw  - begin (%d, %d)\n", da_bx, da_by); */
    /* printf("draw  - end   (%d, %d)\n", da_ex, da_ey); */

    /* (x, y) is points of display */
    /* (bx, by) is points of buffer in window */
    for (int32_t y = da_by, by = 0; y < da_ey && by < w->size.y; y++, by++) {
        const int32_t base = y * display_size.y;
        const int32_t bbase = by * w->size.y;
        for (int32_t x = da_bx, bx = 0; x < da_ex && bx < w->size.x; x++, bx++) {
            RGB8 const c = w->buf[bx + bbase];
            if (c.bit_expr != 0) {
                display_buffer[x + base] = c;
            }
        }
    }

    w->dirty = false;
    is_re_buffer = true;

    return false;
}


void update_windows(void) {
    list_for_each(&win_man->win_list, update_win_for_each, false);
    if (is_re_buffer == false) {
        return;
    }
    is_re_buffer = false;

    int32_t start_x = (begin_p.x < 0) ? (0) : (begin_p.x);
    int32_t start_y = (begin_p.y < 0) ? (0) : (begin_p.y);
    int32_t end_x = (display_size.x < end_p.x) ? (display_size.x) : (end_p.x);
    int32_t end_y = (display_size.y < end_p.y) ? (display_size.y) : (end_p.y);

    /* printf("start (%d, %d)\n", start_x, start_y); */
    /* printf("end   (%d, %d)\n", end_x, end_y); */

    for (int32_t y = start_y; y < end_y ; y++) {
        const int32_t base = y * display_size.y;
        for (int32_t x = start_x; x < end_x ; x++) {
            set_vram(x, y, &display_buffer[base + x]);
        }
    }
}


void move_window_abs(Window* w, Point2d const * p) {
    if (0 < (w->pos.x - p->x)) {
        /* to right */
        begin_p.x = w->pos.x + p->x;
    } else {
        begin_p.x = w->pos.x + p->x;
    }
}


void move_window_rel(Window* const w, Point2d const* const p) {
    /* move_window_abs(w, &make_point2d(w->pos.x + p->x, w->pos.y + p->y)); */
    int32_t const dx = p->x;
    int32_t const dy = p->y;
    int32_t const px = w->pos.x;
    int32_t const py = w->pos.y;
    int32_t const sx = w->size.x;
    int32_t const sy = w->size.y;

    /* update value in argument window. */
    w->pos.x += dx;
    w->pos.y += dy;

    if (0 < dx) {
        /* to right */
        begin_p.x = px;
        end_p.x = px + sx + dx;
    } else {
        /* to left */
        begin_p.x = px + dx;          // px - (-dx)
        end_p.x = px + sx;
    }

    if (0 < dy) {
        /* to below */
        begin_p.y = py;
        end_p.y = py + sy + dy;
    } else {
        /* to above */
        begin_p.y = py + dy;          // py - (-dy)
        end_p.y = py + sy;
    }
    update_windows();

    /* w->dirty = true; */

    begin_p = w->pos;
    end_p.x = w->pos.x + sx;
    end_p.y = w->pos.y + sy;

    update_windows();

    /* clear_point2d(&begin_p); */
    /* end_p = display_size; */
    /* update_windows(); */
}
