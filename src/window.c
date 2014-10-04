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
#include <state_code.h>
#include <utils.h>
#include <font.h>


struct window_manager {
    Elist windows;
    Point2d display_size; /* This storing display size. */
    struct {
        Point2d begin, end;
    } check_area;         /* This is checking area in update_win_for_each. */
    RGB8* display_buffer; /* For double buffering. */
    Window* mouse_win;    /* This is always top on other windows. */
};
typedef struct window_manager Window_manager;


static Window_manager* win_man; /* Window manager. */

static void calibrate(Point2d* const p);
static void update_window_buffer(void);
static void update_window_vram(void);
static void flush_area(Point2d const* const, Point2d const* const);



Axel_state_code init_window(void) {
    win_man = vmalloc_zeroed(sizeof(Window_manager));
    if (NULL == win_man) {
        return AXEL_MEMORY_ALLOC_ERROR;
    }
    win_man->display_size = make_point2d(get_max_x_resolution(), get_max_y_resolution());

    win_man->display_buffer = vmalloc_zeroed(sizeof(RGB8) * (size_t)(win_man->display_size.x * win_man->display_size.y));
    if (win_man->display_buffer == NULL) {
        vfree(win_man);
        return AXEL_MEMORY_ALLOC_ERROR;
    }

    /* alloc mouse window */
    Window* mouse_win = vmalloc_zeroed(sizeof(Window));
    if (mouse_win == NULL) {
        vfree(win_man->display_buffer);
        vfree(win_man);
        return AXEL_MEMORY_ALLOC_ERROR;
    }
    mouse_win->buf = vmalloc_zeroed(sizeof(RGB8) * (size_t)(mouse_cursor->width * mouse_cursor->height));
    mouse_win->pos = make_point2d(get_max_x_resolution() / 2, get_max_y_resolution() / 2);
    mouse_win->size = make_point2d(mouse_cursor->width, mouse_cursor->height);
    mouse_win->lock = false;
    mouse_win->dirty = true;
    mouse_win->enable = true;
    mouse_win->has_inv_color = true;
    mouse_win->reserved = 0;

    if (mouse_win->buf == NULL) {
        vfree(win_man->display_buffer);
        vfree(win_man);
        vfree(mouse_win);
        return AXEL_MEMORY_ALLOC_ERROR;
    }
    window_draw_bitmap(mouse_win, mouse_cursor, 2);

    elist_init(&win_man->windows);
    elist_insert_next(&win_man->windows, &mouse_win->list);

    win_man->mouse_win = (Window*)(win_man->windows.next);

    return AXEL_SUCCESS;
}


Window* get_mouse_window(void) {
    return (Window*)win_man->mouse_win;
}


Window* alloc_window(Point2d const* pos, Point2d const* size, uint8_t level) {
    Window* w = vmalloc_zeroed(sizeof(Window));
    if (w == NULL) {
        return NULL;
    }

    if (size->x <= 0 || size->y <= 0) {
        /* invalid size. */
        return NULL;
    }

    w->pos = *pos;
    w->size = *size;

    calibrate(&w->pos);
    calibrate(&w->size);

    w->buf = vmalloc_zeroed(sizeof(RGB8) * (size_t)(w->size.x * w->size.y));
    if (w->buf == NULL) {
        vfree(w);
        return NULL;
    }

    /* NOTE: all window is leaved behind mouse. */
    elist_insert_prev(&win_man->mouse_win->list, &w->list);

    return w;
}


Window* alloc_drawn_window(Point2d const* pos, Drawable_bitmap const* dw, size_t len) {
    Window* w = alloc_window(pos, &make_point2d(dw->width, dw->width), 0);
    if (w == NULL) {
        return NULL;
    }

    window_draw_bitmap(w, dw, 100);

    return w;
}


Window* alloc_filled_window(Point2d const* pos, Point2d const* size, uint8_t level, RGB8 const* c) {
    Window* w = alloc_window(pos, size, level);
    if (w == NULL) {
        return NULL;
    }

    /* fill area. */
    int32_t const limit_y = w->size.y;
    int32_t const limit_x = w->size.x;
    for (int32_t y = 0; y < limit_y; y++) {
        RGB8* const b = &w->buf[y * limit_x];
        for (int32_t x = 0; x < limit_x; x++) {
            b[x] = *c;
        }
    }

    return w;
}


void flush_windows(void) {
    static Point2d const origin = {0, 0};
    flush_area(&origin, &win_man->display_size);
}


void move_window(Window* const w, Point2d const* const p) {
    int32_t const dx = p->x;
    int32_t const dy = p->y;
    int32_t const px = w->pos.x;
    int32_t const py = w->pos.y;
    int32_t const sx = w->size.x;
    int32_t const sy = w->size.y;

    if (dx == 0 && dy == 0) {
        return;
    }

    /* FIXME: Window which will move must be mouse below(list last). */
    // if ((Window*)win_man->win_list.node->prev->data != w) {
    //     List_node* n = list_search_node(&win_man->win_list, w);
    //     if (n == NULL) {
    //         /* maybe error. */
    //         return;
    //     }

    //     /* exchange pointer to window. */
    //     void* t = win_man->win_list.node->prev->data;
    //     win_man->win_list.node->prev->data = n->data;
    //     n->data = t;
    // }

    /* update value in argument window. */
    w->pos.x += dx;
    w->pos.y += dy;

    Point2d begin_p, end_p;
    /* calculate all changed area to apply the window buffer to vram. */
    if (0 < dx) {
        /* to right */
        begin_p.x = px;
        end_p.x = px + sx + dx;
    } else {
        /* to left */
        begin_p.x = px + dx;  // px - (-dx)
        end_p.x = px + sx;
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

    calibrate(&begin_p);
    calibrate(&end_p);

    win_man->check_area.begin = begin_p;
    win_man->check_area.end = end_p;

    update_window_buffer();
    update_window_vram();
}


// void updown_window(Window const* const w, bool is_down) {
// }


Window* window_fill_area(Window* const w, Point2d const* const pos, Point2d const* const size, RGB8 const* const c) {
    Point2d const p   = *pos;
    Point2d const s   = *size;
    Point2d const ws  = w->size;
    Point2d const end = {
        w->pos.x + w->size.x,
        w->pos.y + w->size.y
    };

    if ((p.x < 0) || (p.y < 0) || (ws.x < p.x) || (ws.y < p.y) || (end.x < p.x + s.x) || (end.y < p.y + s.y)) {
        /* invalid arguments. */
        return NULL;
    }

    int32_t limit_x = p.x + s.x;
    int32_t limit_y = p.y + s.y;
    for (int32_t y = p.y; y < limit_y; y++) {
        RGB8* const b = &w->buf[y * w->size.x];
        for (int32_t x = p.x; x < limit_x; x++) {
            b[x] = *c;
        }
    }

    /* convert display point. */
    flush_area(&make_point2d(w->pos.x + p.x, w->pos.y + p.y), &s);

    return w;
}


void window_draw_bitmap(Window* const w, Drawable_bitmap const* dw, size_t len) {
    if (w == NULL || dw == NULL || len == 0) {
        return;
    }

    for (int k = 0; k < len; k++) {
        for (int32_t y = 0; y < dw[k].height; y++) {
            for (int32_t x = 0; x < dw[k].width; x++) {
                /* if bit is 1, draw color */
                if (1 == (0x01 & (dw[k].data[y] >> (dw[k].width - x - 1)))) {
                    w->buf[x + w->size.x * y] = dw[k].color;
                }
            }
        }
    }
}


void window_draw_line(Window* const w, Point2d const* const begin, Point2d const* const end, RGB8 const* const c, int32_t bold) {
    /* adjusted begin and end. */
    Point2d abegin = make_point2d(((begin->x < 0) ? (0) : (begin->x)), ((begin->y < 0) ? (0) : (begin->y)));
    Point2d aend   = make_point2d(((w->size.x < end->x) ? (w->size.x) : (end->x)), ((w->size.y < end->y) ? (w->size.y) : (end->y)));

    /* making end points always right of begin. */
    if (aend.x < abegin.x) {
        /* swap */
        Point2d t = abegin;
        abegin    = aend;
        aend      = t;
    }

    int32_t dx = aend.x - abegin.x;
    int32_t dy = aend.y - abegin.y;

    if ((dx == 0 && dy == 0) || bold == 0) {
        /* This case is NOT a line. */
        return;
    }

    int32_t size_x = w->size.x;
    int32_t d = bold > 1;  // (bold / 2)
    if (dx == 0) {
        /* vertical line. */
        abegin.x -= d;
        aend.x += d;
        /* this is mod 2 = 0 that means bold is even number. */
        if ((bold & 0x03) == 0) {
            aend.x -= 1;
        }
        if (abegin.x < 0) {
            abegin.x = 0;
        }
        if (w->size.x < abegin.x) {
            abegin.x = w->size.x;
        }
        printf("abegin (%d, %d)\n", abegin.x, abegin.y);
        printf("aend   (%d, %d)\n", aend.x, aend.y);

        for (int32_t y = abegin.y; y < aend.y; y++) {
            RGB8* const b = &w->buf[y * size_x];
            for (int x = abegin.x; x <= aend.x; x++) {
                b[x] = *c;
            }
        }
    } else if (dy == 0) {
        /* horizontal line. */
        abegin.y -= d;
        aend.y += d;
        /* this is mod 2 = 0 that means bold is even number. */
        if ((bold & 0x03) == 0) {
            aend.y -= 1;
        }
        if (abegin.y < 0) {
            abegin.y = 0;
        }
        if (w->size.y < abegin.y) {
            abegin.y = w->size.y;
        }

        for (int32_t y = abegin.y; y <= aend.y; y++) {
            RGB8* const b = &w->buf[y * size_x];
            for (int x = abegin.x; x < aend.x; x++) {
                b[x] = *c;
            }
        }
    }

    for (int i = 0; i < bold; i++) {
        /* Using Bresenham's line algorithm. */
        int32_t diff = (2 * dy) - dx;
        int32_t direction = ((0 < dy) ? (1) : (-1));    /* Is line right down(1) or right up(-1) ? */
        w->buf[abegin.x + (abegin.y * w->size.x)] = *c; /* plot first point. */
        for (int32_t x = abegin.x + 1, y = abegin.y; x < aend.x; x++) {
            if (0 < (diff * direction)) {
                y += direction;
                diff += 2 * dy - 2 * dx;
            } else {
                diff += 2 * dy;
            }

            w->buf[x + (y * w->size.x)] = *c;
        }

        abegin.y += direction;
        aend.y += direction;
    }
}


static inline void calibrate(Point2d* const p) {
    p->x = ((p->x < 0) ? (0) : ((win_man->display_size.x < p->x) ? (win_man->display_size.x) : (p->x)));
    p->y = ((p->y < 0) ? (0) : ((win_man->display_size.y < p->y) ? (win_man->display_size.y) : (p->y)));
}


static inline void update_window_buffer(void) {
    elist_foreach(w, &win_man->windows, Window, list) {
        /* NOTE: These point is NOT points of buffer and these is display points. */
        /* NOTE: This function supposes that "begin_p" and "end_p" is already calibrated to accelerate. */

        /* Check area points. */
        int32_t const ca_bx = win_man->check_area.begin.x;
        int32_t const ca_by = win_man->check_area.begin.y;
        int32_t const ca_ex = win_man->check_area.end.x;
        int32_t const ca_ey = win_man->check_area.end.y;

        Point2d wbp = w->pos;
        Point2d wep = {w->pos.x + w->size.x, w->pos.y + w->size.y};
        calibrate(&wbp);
        calibrate(&wep);

        /* Window area points. */
        int32_t wa_bx = wbp.x;
        int32_t wa_by = wbp.y;
        int32_t wa_ex = wep.x;
        int32_t wa_ey = wep.y;

        /* checking Is this window included ? */
        if (ca_ex < wa_bx || ca_ey < wa_by || wa_ex < ca_bx || wa_ey < ca_by) {
            continue;
        }

        /* consider lapping between check area and this window area. */
        int32_t da_bx;
        int32_t da_by;
        int32_t da_ex;
        int32_t da_ey;

        if ((ca_bx <= wa_bx) && (ca_by <= wa_by) && (wa_ex <= ca_ex) && (wa_ey <= ca_ey)) {
            /* Check area contains this window area. */
            da_bx = wa_bx;
            da_by = wa_by;
            da_ex = wa_ex;
            da_ey = wa_ey;
        } else if ((wa_bx <= ca_bx) && (wa_by <= ca_by) && (ca_ex <= wa_ex) && (ca_ey <= wa_ey)) {
            /* This window area contains check area. */
            da_bx = ca_bx;
            da_by = ca_by;
            da_ex = ca_ex;
            da_ey = ca_ey;
        } else {
            /* Is window area on the right side or left side against check area ? */
            da_bx = (ca_bx <= wa_bx) ? (wa_bx) : (ca_bx);
            da_ex = (ca_ex <= wa_ex) ? (ca_ex) : (wa_ex);

            /* Is window area on the below or above against check area ? */
            da_by = (ca_by <= wa_by) ? (wa_by) : (ca_by);
            da_ey = (ca_ey <= wa_ey) ? (ca_ey) : (wa_ey);
        }

        /* Begin and end position of display. */
        int32_t const dbegin_x = da_bx;
        int32_t const dbegin_y = da_by;
        int32_t const dend_x = da_ex;
        int32_t const dend_y = da_ey;

        /*
         * Begin and end position of buffer in this window.
         * First, converting display coordinate to buffer coordinate.
         * Then adding position difference to consider specifically case that there are window outside of display.
         */
        int32_t const diff_x   = (w->pos.x < 0) ? (-w->pos.x) : (0);
        int32_t const diff_y   = (w->pos.y < 0) ? (-w->pos.y) : (0);
        int32_t const bbegin_x = (da_bx - wa_bx) + diff_x;
        int32_t const bbegin_y = (da_by - wa_by) + diff_y;
        int32_t const bend_x   = (da_ex - wa_bx) + diff_x;
        int32_t const bend_y   = (da_ey - wa_by) + diff_y;

        /*
         * (bx, by) is points of buffer in window.
         * (dx, dy) is points of display.
         */
        int32_t const win_size_x = w->size.x;
        if (w->has_inv_color == true) {
            for (int32_t by = bbegin_y, dy = dbegin_y; (by < bend_y) && (dy < dend_y); by++, dy++) {
                /* these variable is to accelerate calculation. */
                RGB8* const db = &win_man->display_buffer[dy * win_man->display_size.x];
                RGB8* const b = &w->buf[by * win_size_x];
                for (int32_t bx = bbegin_x, dx = dbegin_x; (bx < bend_x) && (dx < dend_x); bx++, dx++) {
                    if (b[bx].bit_expr != inv_color.bit_expr) {
                        db[dx] = b[bx];
                    }
                }
            }
        } else {
            for (int32_t by = bbegin_y, dy = dbegin_y; (by < bend_y) && (dy < dend_y); by++, dy++) {
                /* these variable is to accelerate calculation. */
                RGB8* const db = &win_man->display_buffer[dy * win_man->display_size.x];
                RGB8* const b = &w->buf[by * win_size_x];
                for (int32_t bx = bbegin_x, dx = dbegin_x; (bx < bend_x) && (dx < dend_x); bx++, dx++) {
                    db[dx] = b[bx];
                }
            }
        }
    }
}


static inline void update_window_vram(void) {
    Point2d const display_size = win_man->display_size;
    int32_t const start_x      = (win_man->check_area.begin.x < 0) ? (0) : (win_man->check_area.begin.x);
    int32_t const start_y      = (win_man->check_area.begin.y < 0) ? (0) : (win_man->check_area.begin.y);
    int32_t const end_x        = (display_size.x < win_man->check_area.end.x) ? (display_size.x) : (win_man->check_area.end.x);
    int32_t const end_y        = (display_size.y < win_man->check_area.end.y) ? (display_size.y) : (win_man->check_area.end.y);

    for (int32_t y = start_y; y < end_y; y++) {
        RGB8* const db = &win_man->display_buffer[y * display_size.x];
        for (int32_t x = start_x; x < end_x; x++) {
            set_vram(x, y, &db[x]);
        }
    }
}


static inline void flush_area(Point2d const* const pos, Point2d const* const size) {
    win_man->check_area.begin = *pos;
    win_man->check_area.end.x = pos->x + size->x;
    win_man->check_area.end.y = pos->y + size->y;
    update_window_buffer();
    update_window_vram();
}
