/**
 * @file include/window.h
 * @brief Window header.
 * @author mopp
 * @version 0.1
 * @date 2014-06-17
 */


#ifndef _WINDOW_H_
#define _WINDOW_H_



#include <point.h>
#include <rgb8.h>
#include <state_code.h>
#include <drawable.h>
#include <stddef.h>
#include <elist.h>


struct window {
    Elist list;
    RGB8* buf;
    Point2d pos;
    Point2d size;
    union {
        struct {
            uint8_t lock : 1;
            uint8_t writable : 1;
            uint8_t enable : 1;
            uint8_t has_inv_color : 1;
            uint8_t reserved : 4;
        };
        uint8_t flags;
    };
    struct {
        RGB8 fg;
        RGB8 bg;
        Point2d wr_begin;
        Point2d wr_size;
        Point2d wr_pos;
    };
};
typedef struct window Window;

/* typedef void (*event_listener)(Window*); */
static RGB8 const inv_color = {.bit_expr = 0};


extern Axel_state_code init_window(void);
extern Window* alloc_window(Point2d const*, Point2d const*);
extern Window* alloc_drawn_window(Point2d const*, Drawable_bitmap const*);
extern Window* alloc_filled_window(Point2d const*, Point2d const*, RGB8 const*);
extern Window* get_mouse_window(void);
extern void window_draw_bitmap(Window* const, Drawable_bitmap const*, size_t);
extern void window_draw_line(Window* const, Point2d const* const, Point2d const* const, RGB8 const* const, int32_t);
extern void free_window(Window*);
extern void flush_windows(void);
extern void move_window(Window* const w, Point2d const* const p);
extern Window* get_top_writable_window(void);
extern void w_putchar(int c);
extern Window* window_set_writable(Window*, RGB8 const*, RGB8 const*, Point2d const*, Point2d const*);
extern Window* window_fill_area(Window* const, Point2d const* const, Point2d const* const, RGB8 const* const);

#endif
