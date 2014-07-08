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
static void flush_area(Point2d const* const, Point2d const* const);



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

    w.buf = vmalloc(sizeof(RGB8) * ((size_t)w.size.x * (size_t)w.size.y));
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
    static Point2d const origin = {0, 0};
    flush_area(&origin, &display_size);
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

    /* Window which will move must be top(list last). */
    if ((Window*)win_man->win_list.node->prev->data != w) {
        List_node* n = list_search_node(&win_man->win_list, w);
        if (n == NULL) {
            /* maybe error. */
            return; 
        }

        /* exchange pointer to window. */
        void* t = win_man->win_list.node->prev->data;
        win_man->win_list.node->prev->data = n->data;
        n->data = t;
    }

    /* update value in argument window. */
    w->pos.x += dx;
    w->pos.y += dy;

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

    update_window_buffer();
    update_window_vram();
}


Window* window_fill_area(Window* const w, Point2d const* const pos, Point2d const* const size, RGB8 const* const c) {
    Point2d const p   = *pos;
    Point2d const s   = *size;
    Point2d const ws  = w->size;
    Point2d const end = {w->pos.x + w->size.x, w->pos.y + w->size.y};

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
    Window* const w = d;

    /* NOTE: These point is NOT points of buffer and these is display points. */
    /* NOTE: This function supposes that "begin_p" and "end_p" is already calibrated to accelerate. */

    /* Check area points. */
    int32_t const ca_bx = begin_p.x;
    int32_t const ca_by = begin_p.y;
    int32_t const ca_ex = end_p.x;
    int32_t const ca_ey = end_p.y;

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
        return false;
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
    int32_t const dend_x   = da_ex;
    int32_t const dend_y   = da_ey;

    /*
     * Begin and end position of buffer in this window.
     * First, converting display coordinate to buffer coordinate.
     * Then adding position difference to consider specifically case that there are window outside of display.
     */
    int32_t const diff_x = (w->pos.x < 0) ? (-w->pos.x) : (0);
    int32_t const diff_y = (w->pos.y < 0) ? (-w->pos.y) : (0);
    int32_t const bbegin_x = (da_bx - wa_bx) + diff_x;
    int32_t const bbegin_y = (da_by - wa_by) + diff_y;
    int32_t const bend_x   = (da_ex - wa_bx) + diff_x;
    int32_t const bend_y   = (da_ey - wa_by) + diff_y;

    /*
     * (bx, by) is points of buffer in window.
     * (dx, dy) is points of display.
     */
    int32_t const win_size_x = w->size.x;
    for (int32_t by = bbegin_y, dy = dbegin_y; (by < bend_y) && (dy < dend_y); by++, dy++) {
        /* these variable is to accelerate calculation. */
        RGB8* const db = &display_buffer[dy * display_size.x];
        RGB8* const b = &w->buf[by * win_size_x];
        for (int32_t bx = bbegin_x, dx = dbegin_x; (bx < bend_x) && (dx < dend_x); bx++, dx++) {
            if (b[bx].bit_expr != 0) {
                db[dx] = b[bx];
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
        RGB8* const db = &display_buffer[y * display_size.x];
        for (int32_t x = start_x; x < end_x; x++) {
            set_vram(x, y, &db[x]);
        }
    }
}


static inline void flush_area(Point2d const* const pos, Point2d const* const size) {
    begin_p = *pos;
    end_p.x = pos->x + size->x;
    end_p.y = pos->y + size->y;
    update_window_buffer();
    update_window_vram();
}
