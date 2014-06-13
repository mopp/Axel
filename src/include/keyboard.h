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

struct keyboard {
    bool enable_keybord;
    bool enable_calps_lock;
    bool enable_num_lock;
    bool enable_scroll_lock;
    Aqueue aqueue;
};
typedef struct keyboard Keyboard;


union keyboard_controle_status {
    struct {
        uint8_t output_buf_full : 1; /* If this is 1, data exists into output register. */
        uint8_t input_buf_full : 1;  /* If this is 1, data exists into input register. may be, controller executing. */
        uint8_t system_flag : 1;     /* This is changed by command result. */
        uint8_t cmd_data_flag : 1;   /* If this is 1, Last writting is command. otherwise, last writting is data */
        uint8_t lock_flag : 1;       /* If this is 0, keyboard is locked. otherwise, not locked. */
        uint8_t read_timeout : 1;
        uint8_t write_timeout : 1;
        uint8_t parity_error : 1;
    };
    uint8_t bit_expr;
};
typedef union keyboard_controle_status Keyboard_controle_status;
_Static_assert(sizeof(Keyboard_controle_status) == 1, "Keyboard_controle_status is NOT 1 byte.");


enum keyboard_constants {
    KEYBOARD_BUFFER_SIZE                = 1024, /* keyboard buffer size */

    KEYBOARD_DATA_PORT                  = 0x60, /* Read: Output from keyboard register, Write: input to keyboard register */
    KEYBOARD_OUTPUT_PORT                = 0x60, /* for Read. */
    KEYBOARD_INPUT_PORT                 = 0x60, /* for Write. */

    KEYBOARD_CONTROL_PORT               = 0x64, /* Read: Status register,  Write: Command register */
    KEYBOARD_STATUS_PORT                = 0x64, /* for Read. */
    KEYBOARD_COMMAND_PORT               = 0x64, /* for Write. */

    KEYBOARD_STATUS_OUTPUT_BUFFER_FULL  = 0x01,
    KEYBOARD_STATUS_INPUT_BUFFER_FULL   = 0x02,
    KEYBOARD_STATUS_SYSTEM_FLAG         = 0x04,
    KEYBOARD_STATUS_CMD_DATA_FLAG       = 0x08,
    KEYBOARD_STATUS_LOCK_FLAG           = 0x10,
    KEYBOARD_STATUS_READ_TIMEOUT        = 0x20,
    KEYBOARD_STATUS_WRITE_TIMEOUT       = 0x40,
    KEYBOARD_STATUS_PARITY_ERROR        = 0x80,

    KEYBOARD_DATA_COMMAND_LED           = 0xED, /* Set LED for scroll, num, caps lock */
    KEYBOARD_DATA_COMMAND_ECHO          = 0xEE, /* return writting data(0xEE) to 0x60 */
    KEYBOARD_DATA_COMMAND_SCANCODE      = 0xF0, /* get or set scan code */
    KEYBOARD_DATA_COMMAND_KEYBOARD_ID   = 0xF2, /* get keyboard id, after writting this command, it takes at least 10ms. */

    KEYBOARD_LED_NUM                    = 0x01,
    KEYBOARD_LED_SCROLL                 = 0x02,
    KEYBOARD_LED_CAPS                   = 0x04,

    KEYBOARD_CONTROL_COMMAND_READ       = 0x20, /* read control command byte from keyboard controller. */
    KEYBOARD_CONTROL_COMMAND_WRITE      = 0x60, /* write control command byte to keyboard controller. */
    KEYBOARD_CONTROL_COMMAND_SELF_TEST  = 0xAA, /* do self test -> 0x55 is OK result, 0xFC is NG. */
    KEYBOARD_CONTROL_COMMAND_READ_INPUT = 0xC0, /* read data from input port, result is in 0x64. */

    KEYBOARD_COMMAND_BYTE_KIE           = 0x01, /* Keyboard interruput enable . */
    KEYBOARD_COMMAND_BYTE_MIE           = 0x02, /* Mouse interruput enable. */
    KEYBOARD_COMMAND_BYTE_SYSF          = 0x04, /* System flag. */
    KEYBOARD_COMMAND_BYTE_IGNLK         = 0x08, /* Ignore keyboard lock. */
    KEYBOARD_COMMAND_BYTE_KE            = 0x10, /* Keyboard enable. */
    KEYBOARD_COMMAND_BYTE_ME            = 0x20, /* Mouse enable. */
    KEYBOARD_COMMAND_BYTE_XLATE         = 0x40, /* Scan code translation enable. */

    KEYBOARD_SELF_TEST_SUCCESS          = 0x55,
    KEYBOARD_SELF_TEST_FAILED           = 0xFC,
};


extern Axel_state_code init_keyboard(void);



#endif
