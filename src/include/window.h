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


enum windows_constants {
    MAX_WINDOW_NUM = 128,
};


/* typedef void (*event_listener)(Window*); */

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
    uint8_t level;  /* 0 is top */
};
typedef struct window Window;


struct window_manager {
    List win_list;
};
typedef struct window_manager Window_manager;



Window* alloc_window(Point2d const*, Point2d const*, uint8_t);
Window* alloc_filled_window(Point2d const*, Point2d const*, uint8_t, RGB8 const*);
Axel_state_code init_window(void);
void free_window(Window*);
void update_windows(void);


#endif
