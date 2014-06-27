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


struct window;
typedef struct window Window;

/* typedef void (*event_listener)(Window*); */


Axel_state_code init_window(void);
Window* alloc_window(Point2d const*, Point2d const*, uint8_t);
Window* alloc_drawn_window(Point2d const* pos, Drawable_bitmap const* dw, size_t len);
Window* alloc_filled_window(Point2d const*, Point2d const*, uint8_t, RGB8 const*);
void free_window(Window*);
void flush_windows(void);
void move_window(Window* const w, Point2d const* const p);
Window* window_fill_area(Window* const, Point2d const* const, Point2d const* const, RGB8 const* const);

#endif
