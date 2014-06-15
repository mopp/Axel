/**
 * @file keyboard.h
 * @brief keyboard header.
 * @author mopp
 * @version 0.1
 * @date 2014-04-11
 */
#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_



#include <state_code.h>
#include <stdint.h>
#include <stdbool.h>
#include <aqueue.h>
#include <point.h>


struct keyboard {
    bool enable_keybord;
    bool enable_calps_lock;
    bool enable_num_lock;
    bool enable_scroll_lock;
    Aqueue aqueue;
};
typedef struct keyboard Keyboard;


struct mouse {
    bool enable_mouse;
    uint8_t button;
    uint8_t phase;
    uint8_t packets[4];
    bool is_pos_update;
    Point2d pos;
    Aqueue aqueue;
};
typedef struct mouse Mouse;


enum mouse_constants {
    /* These value corresponds to mouse first packet. */
    MOUSE_BUTTON_NONE   = 0x00,
    MOUSE_BUTTON_MIDDLE = 0x04,
    MOUSE_BUTTON_RIGHT  = 0x02,
    MOUSE_BUTTON_LEFT   = 0x01,
};



extern Axel_state_code init_keyboard(void);
extern Axel_state_code init_mouse(void);



#endif
