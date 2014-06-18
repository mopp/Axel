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


static Window_manager* win_man;
static Point2d display_size;



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


static uint8_t level_in_for_each;
static bool alloc_for_each_func(void* d) {
    Window const* w = (Window*)d;
    return (w->level == level_in_for_each || level_in_for_each < w->level) ? true : false;
}


Window* alloc_window(Point2d const* pos, Point2d const* size, uint8_t level) {
    Window w = {.lock = false, .dirty = true, .enable = true, .reserved = 0};

    w.pos.x = pos->x;
    w.pos.y = pos->y;
    w.size.x = size->x;
    w.size.y = size->y;
    w.level = level;

    calibrate(&w.pos);
    calibrate(&w.size);

    w.buf = vmalloc(sizeof(RGB8) * (size_t)w.size.x * (size_t)w.size.y);
    if (w.buf == NULL) {
        return NULL;
    }

    level_in_for_each = level;
    List_node* n = list_for_each(&win_man->win_list, alloc_for_each_func, false);
    if (n == NULL) {
        list_insert_data_last(&win_man->win_list, &w);
        return win_man->win_list.node->prev->data;
    }

    Window* wp = n->data;
    if (wp->level == level) {
        /* new window */
        n = list_insert_data_prev(&win_man->win_list, n, &w);
    } else {
        /* In this case, wp->level < level */
        n = list_insert_data_prev(&win_man->win_list, n, &w);
    }

    return n->data;
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


Axel_state_code init_window(void) {
    win_man = vmalloc(sizeof(Window_manager));
    if (NULL == win_man) {
        return AXEL_MEMORY_ALLOC_ERROR;
    }
    list_init(&win_man->win_list, sizeof(Window), win_node_free);

    display_size.x = get_max_x_resolution();
    display_size.y = get_max_y_resolution();

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

    if (w->enable == false || w->dirty == false) {
        return false;
    }
    printf("level: %d\n", w->level);

    for (int i = 0; i < w->size.y; i++) {
        for (int j = 0; j < w->size.x; j++) {
            set_vram(w->pos.x + j, w->pos.y + i, &w->buf[j + w->size.y * i]);
        }
    }

    w->dirty = false;

    return false;
}


void update_windows(void) {
    list_for_each(&win_man->win_list, update_win_for_each, false);
}
