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


#include <stdio.h>
Axel_state_code init_window(void) {
    display_size.x = get_max_x_resolution();
    display_size.y = get_max_y_resolution();
    size_t const display_pixel = (size_t)(display_size.x * display_size.y);
    printf("%zdKB\n", display_pixel / 1024);

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


static bool is_re_buffer;
static uint8_t cnt;
static bool update_win_for_each(void* d) {
    Window* w = d;

    if (w->dirty == false || w->enable == false) {
        ++cnt;
        return false;
    }
    calibrate(&w->pos);
    calibrate(&w->size);
    printf("(x, y) = (%d, %d)\n", w->pos.x, w->pos.y);

    int32_t const mstart_y = w->pos.y;
    int32_t const mstart_x = w->pos.x;
    int32_t const mlimit_y = w->pos.y + w->size.y;
    int32_t const mlimit_x = w->pos.x + w->size.x;
    int32_t const bstart_y = 0;
    int32_t const bstart_x = 0;
    int32_t const blimit_y = w->size.y;
    int32_t const blimit_x = w->size.x;

    for (int32_t mi = mstart_y, bi = bstart_y; mi < mlimit_y && bi < blimit_y; mi++, bi++) {
        int32_t const mbase = mi * display_size.y;
        int32_t const bbase = bi * blimit_y;
        for (int32_t mj = mstart_x, bj = bstart_x; mj < mlimit_x && bj < blimit_x; mj++, bj++) {
            if (map[mbase + mj] == UINT8_MAX) {
                map[mbase + mj] = cnt;
                display_buffer[mbase + mj] = w->buf[bbase + bj];
            }
        }
    }

    w->dirty = false;
    is_re_buffer = true;
    ++cnt;

    return false;
}


void update_windows(void) {
    cnt = 0;
    list_for_each(&win_man->win_list, update_win_for_each, false);
    if (is_re_buffer == false) {
        return;
    }
    is_re_buffer = false;

    for (int32_t i = 0; i < display_size.y; i++) {
        int32_t const base = i * display_size.y;
        for (int32_t j = 0; j < display_size.x; j++) {
            if (map[base + j] != UINT8_MAX) {
                set_vram(j, i, &display_buffer[base + j]);
            }
        }
    }
}
