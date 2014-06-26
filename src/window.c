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


static Window_manager* win_man; /* Window manager. */
static Point2d display_size;    /* This storing display size. */
static RGB8* display_buffer;    /* For double buffering. */
static Point2d begin_p, end_p;  /* This is checking area in update_win_for_each. */

static void calibrate(Point2d* const p);
static void win_node_free(void*);
static void update_window_buffer(void);
static void update_window_vram(void);



Axel_state_code init_window(void) {
    display_size.x = get_max_x_resolution();
    display_size.y = get_max_y_resolution();

    size_t const display_pixel = (size_t)(display_size.x * display_size.y);

    display_buffer = vmalloc(sizeof(RGB8) * display_pixel);
    if (display_buffer == NULL) {
        return AXEL_MEMORY_ALLOC_ERROR;
    }
    memset(display_buffer, 0, sizeof(RGB8) * display_pixel);

    win_man = vmalloc(sizeof(Window_manager));
    if (NULL == win_man) {
        return AXEL_MEMORY_ALLOC_ERROR;
    }
    list_init(&win_man->win_list, sizeof(Window), win_node_free);

    return AXEL_SUCCESS;
}


Window* alloc_window(Point2d const* pos, Point2d const* size, uint8_t level) {
    Window w = {.lock = false, .dirty = true, .enable = true, .reserved = 0};

    if (size->x <= 0 || size->y <= 0) {
        /* invalid size. */
        return NULL;
    }

    w.pos = *pos;
    w.size = *size;

    calibrate(&w.pos);
    calibrate(&w.size);

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


Window* alloc_drawn_window(Point2d const* pos, Drawable_bitmap const* dw, size_t len) {
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


void flush_windows(void) {
    clear_point2d(&begin_p);
    end_p = display_size;
    update_window_buffer();
    update_window_vram();
}


void move_window(Window* const w, Point2d const* const p) {
    int32_t const dx = p->x;
    int32_t const dy = p->y;
    int32_t const px = w->pos.x;
    int32_t const py = w->pos.y;
    int32_t const sx = w->size.x;
    int32_t const sy = w->size.y;

    /* update value in argument window. */
    w->pos.x += dx;
    w->pos.y += dy;

    /* update buffer before area */
    if (0 < dx) {
        /* to right */
        begin_p.x = px;
        end_p.x = px + dx;
    } else {
        /* to left */
        begin_p.x = px + sx + dx;  // px + sx - (-dx)
        end_p.x = px + sx;
    }
    if (0 < dy) {
        /* to below */
        begin_p.y = py + sy;
        end_p.y = py + sy + dy;
    } else {
        /* to above */
        begin_p.y = py + dy;  // py - (-dy)
        end_p.y = py;
    }
    update_window_buffer();

    /* update buffer after area */
    begin_p = w->pos;
    end_p.x = w->pos.x + sx;
    end_p.y = w->pos.y + sy;
    update_window_buffer();

    /* calculate all changed area to apply the window buffer to vram. */
    if (0 < dx) {
        /* to right */
        begin_p.x = px;
        end_p.x = px + sx + dx;
    } else {
        /* to left */
        begin_p.x = px + dx;     // px - (-dx)
        end_p.x = px + sx - dx;  // px + sx + (-dx)
    }

    if (0 < dy) {
        /* to below */
        begin_p.y = py;
        end_p.y = py + sy + dy;
    } else {
        /* to above */
        begin_p.y = py + dy;  // py - (-dy)
        end_p.y = py + sy;
    }

    update_window_vram();
}


static void win_node_free(void* d) {
    if (d == NULL) {
        return;
    }

    Window* w = (Window*)d;

    vfree(w->buf);
    vfree(w);
}


static inline void calibrate(Point2d* const p) {
    p->x = ((p->x < 0) ? (0) : ((display_size.x < p->x) ? (display_size.x) : (p->x)));
    p->y = ((p->y < 0) ? (0) : ((display_size.y < p->y) ? (display_size.y) : (p->y)));
}


static bool update_win_for_each(void* d) {
    Window* w = d;

    if (w->enable == false) {
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

    /* checking Is this window included ? */
    if (!((ca_bx <= ba_bx || ba_bx <= ca_ex) || (ca_by <= ba_by || ba_by <= ca_ey))) {
        return false;
    }

    /* consider lapping between check area and this window area. */
    int32_t da_bx = 0;
    int32_t da_by = 0;
    int32_t da_ex = 0;
    int32_t da_ey = 0;

    if ((ca_bx <= ba_bx) && (ca_by <= ba_by) && (ba_ex <= ca_ex) && (ba_ey <= ca_ey)) {
        /* Check area contains this buffer area. */
        da_bx = ba_bx;
        da_by = ba_by;
        da_ex = ba_ex;
        da_ey = ba_ex;
    } else if ((ba_bx <= ca_bx) && (ba_by <= ca_by) && (ca_ex <= ba_ex) && (ca_ey <= ba_ey)) {
        /* This buffer area contains check area. */
        da_bx = ca_bx;
        da_by = ca_by;
        da_ex = ca_ex;
        da_ey = ca_ex;
    } else {
        if (ca_bx <= ba_bx) {
            /* buffer area is on the right side of check area. */
            da_bx = ba_bx;
            da_ex = ca_ex;
        } else {
            /* buffer area is on the left side of check area. */
            da_bx = ca_bx;
            da_ex = ba_ex;
        }

        if (ca_by <= ba_by) {
            /* buffer area is below check area. */
            da_by = ba_by;
            da_ey = ca_ey;
        } else {
            /* buffer area is above check area. */
            da_by = ca_by;
            da_ey = ba_ey;
        }
    }

    /*
     * (x, y) is points of display
     * (bx, by) is points of buffer in window
     */
    int32_t const limit_x = w->size.x;
    int32_t const limit_y = w->size.y;
    for (int32_t by = 0; by < limit_y; by++) {
        int32_t const base = by * limit_y;
        int32_t const dbase = (ba_by + by) * display_size.y;
        for (int32_t bx = 0; bx < limit_x; bx++) {
            RGB8 const c = w->buf[bx + base];
            if (c.bit_expr != 0) {
                display_buffer[dbase + (ba_bx + bx)] = c;
            }
        }
    }

    return false;
}


static inline void update_window_buffer(void) {
    list_for_each(&win_man->win_list, update_win_for_each, false);
}


static inline void update_window_vram(void) {
    int32_t const start_x = (begin_p.x < 0) ? (0) : (begin_p.x);
    int32_t const start_y = (begin_p.y < 0) ? (0) : (begin_p.y);
    int32_t const end_x = (display_size.x < end_p.x) ? (display_size.x) : (end_p.x);
    int32_t const end_y = (display_size.y < end_p.y) ? (display_size.y) : (end_p.y);

    for (int32_t y = start_y; y < end_y; y++) {
        int32_t const base = y * display_size.y;
        for (int32_t x = start_x; x < end_x; x++) {
            set_vram(x, y, &display_buffer[base + x]);
        }
    }
}
