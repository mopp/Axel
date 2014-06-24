/**
 * @file window.h
 * @brief window header.
 * @author mopp
 * @version 0.1
 * @date 2014-06-17
 */


#ifndef _WINDOW_H_
#define _WINDOW_H_



#include <list.h>
#include <point.h>
#include <rgb8.h>
#include <state_code.h>
#include <drawable.h>


enum windows_constants {
    MAX_WINDOW_NUM = 128,
};


struct window;
typedef struct window Window;

/* typedef void (*event_listener)(Window*); */



Window* alloc_window(Point2d const*, Point2d const*, uint8_t);
Window* alloc_filled_window(Point2d const*, Point2d const*, uint8_t, RGB8 const*);
Axel_state_code init_window(void);
void free_window(Window*);
void update_windows(void);
Window* alloc_drawn_window(Point2d const* pos, Drawable_bitmap const * dw, size_t len);
void move_window_rel(Window* const w, Point2d const* const p);

#endif
