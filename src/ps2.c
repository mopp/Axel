/**
 * @file keyboard.c
 * @brief related keyboard functions.
 * @author mopp
 * @version 0.1
 * @date 2014-04-11
 */
#include <ps2.h>
#include <point.h>
#include <asm_functions.h>
#include <graphic.h>
#include <interrupt.h>
#include <utils.h>
#include <kernel.h>
#include <macros.h>
#include <paging.h>


#define FAILED_RETURN(exp)    \
    if (exp == AXEL_FAILED) { \
        return AXEL_FAILED;   \
    }


union keyboard_status_flags {
    struct {
        /*
         * If this is 1, data exists into output register and must be
         * must be set before attempting to read data from IO port 0x60
         */
        uint8_t output_buf_full : 1;
        /*
         * If this is 1, data exists into input register. may be, controller executing.
         * must be clear before attempting to write data to IO port 0x60 or IO port 0x64
         */
        uint8_t input_buf_full : 1;
        uint8_t system_flag : 1;     /* This is changed by command result. */
        uint8_t cmd_data_flag : 1;   /* If this is 1, Last writting is command. otherwise, last writting is data */
        uint8_t lock_flag : 1;       /* If this is 0, keyboard is locked. otherwise, not locked. */
        uint8_t mouse_data_came : 1; /* If this is 1, there are mouse data at KEYBOARD_STATUS_PORT. otherwise, there are keyboard data. */
        uint8_t timeout_error : 1;
        uint8_t parity_error : 1;
    };
    uint8_t bit_expr;
};
typedef union keyboard_status_flags Keyboard_status_flags;
_Static_assert(sizeof(Keyboard_status_flags) == 1, "Keyboard_status_flags is NOT 1 byte.");


union keyboard_config_byte {
    struct {
        uint8_t keyboard_interrupt_enable : 1;
        uint8_t mouse_interrupt_enable : 1;
        uint8_t system_flag : 1;
        uint8_t zero1 : 1;
        uint8_t keyboard_disable : 1;
        uint8_t mouse_disable : 1;
        uint8_t scancode_trans_enable : 1;
        uint8_t zero2 : 1;
    };
    uint8_t bit_expr;
};
typedef union keyboard_config_byte Keyboard_config_byte;
_Static_assert(sizeof(Keyboard_config_byte) == 1, "Keyboard_config_byte is NOT 1 byte.");


enum ps2_constants {
    KEYBOARD_BUFFER_SIZE               = 128,  /* keyboard buffer size */
    MOUSE_BUFFER_SIZE                  = 128,  /* mouse buffer size */

    KEYBOARD_DATA_PORT                 = 0x60, /* Read: Output from keyboard register, Write: input to keyboard register */
    KEYBOARD_STATUS_PORT               = 0x64, /* Read: Status register. */
    KEYBOARD_COMMAND_PORT              = 0x64, /* Write: Command register. */

    KEYBOARD_STATUS_OUTPUT_BUFFER_FULL = 0x01,
    KEYBOARD_STATUS_INPUT_BUFFER_FULL  = 0x02,
    KEYBOARD_STATUS_SYSTEM_FLAG        = 0x04,
    KEYBOARD_STATUS_CMD_DATA_FLAG      = 0x08,
    KEYBOARD_STATUS_LOCK_FLAG          = 0x10,
    KEYBOARD_STATUS_READ_TIMEOUT       = 0x20,
    KEYBOARD_STATUS_WRITE_TIMEOUT      = 0x40,
    KEYBOARD_STATUS_PARITY_ERROR       = 0x80,

    KEYBOARD_DATA_CMD_LED              = 0xED, /* Set LED for scroll, num, caps lock */
    KEYBOARD_DATA_CMD_ECHO             = 0xEE, /* return writting data(0xEE) to 0x60 */
    KEYBOARD_DATA_CMD_SCANCODE         = 0xF0, /* get or set scan code */
    KEYBOARD_DATA_CMD_KEYBOARD_ID      = 0xF2, /* get keyboard id, after writting this command, it takes at least 10ms. */
    SCANCODE_1                         = 0x43,
    SCANCODE_2                         = 0x41,
    SCANCODE_3                         = 0x3f,

    KEYBOARD_LED_SCROLL                = 0x01,
    KEYBOARD_LED_NUM                   = 0x02,
    KEYBOARD_LED_CAPS                  = 0x04,

    KEYBOARD_CTRL_CMD_READ             = 0x20, /* read control command byte from keyboard controller. */
    KEYBOARD_CTRL_CMD_WRITE            = 0x60, /* write control command byte to keyboard controller. */
    KEYBOARD_CTRL_CMD_SELF_TEST        = 0xAA, /* do self test -> 0x55 is OK result, 0xFC is NG. */
    KEYBOARD_CTRL_CMD_INTERFACE_TEST   = 0xAB,
    KEYBOARD_CTRL_CMD_DISABLE          = 0xAD, /* Disable keyboard. */
    KEYBOARD_CTRL_CMD_ENABLE           = 0xAE, /* Enable keyboard. */
    KEYBOARD_CTRL_CMD_DISABLE_MOUSE    = 0xA7, /* Disable mouse. */
    KEYBOARD_CTRL_CMD_ENABLE_MOUSE     = 0xA8, /* Enable mouse. */
    KEYBOARD_CTRL_CMD_READ_INPUT       = 0xC0, /* read data from input port, result is in 0x64. */
    KEYBOARD_CTRL_CMD_MOUSE_TEST       = 0xA9, /* do mouse test, result is in 0x60. */
    KEYBOARD_CTRL_CMD_WRITE_MOUSE      = 0xD4, /* write data to mouse. */

    KEYBOARD_CMD_BYTE_KIE              = 0x01, /* Keyboard interruput enable . */
    KEYBOARD_CMD_BYTE_MIE              = 0x02, /* Mouse interruput enable. */
    KEYBOARD_CMD_BYTE_SYSF             = 0x04, /* System flag. */
    KEYBOARD_CMD_BYTE_IGNLK            = 0x08, /* Ignore keyboard lock. */
    KEYBOARD_CMD_BYTE_KE               = 0x10, /* Keyboard enable. */
    KEYBOARD_CMD_BYTE_ME               = 0x20, /* Mouse enable. */
    KEYBOARD_CMD_BYTE_XLATE            = 0x40, /* Scan code translation enable. */

    MOUSE_COMMAND_BYTE_RESET           = 0xFF,
    MOUSE_COMMAND_BYTE_RESEND          = 0xFE,
    MOUSE_COMMAND_BYTE_DEFAULT         = 0xF6,
    MOUSE_COMMAND_BYTE_DISABLE_STREAM  = 0xF5,
    MOUSE_COMMAND_BYTE_ENABLE_STREAM   = 0xF4,
    MOUSE_COMMAND_BYTE_SET_RATE        = 0xF3,
    MOUSE_COMMAND_BYTE_GET_ID          = 0xF2,
    MOUSE_COMMAND_BYTE_GET_ONE_PACKET  = 0xEB,
    MOUSE_COMMAND_BYTE_GET_STATUS      = 0xE9,
    MOUSE_COMMAND_BYTE_SET_RESOLUTION  = 0xE8,

    KEYBOARD_SELF_TEST_SUCCESS         = 0x55,
    KEYBOARD_SELF_TEST_FAILED          = 0xFC,
    KEYBOARD_INTERFACE_TEST_SUCCESS    = 0x00,
    MOUSE_TEST_SUCCESS                 = 0x00,

    PS2_ACKNOWLEDGE                    = 0xFA,
    PS2_RESEND                         = 0xFE,
    PS2_RESET_CMD                       = 0xFF,
};



static Keyboard keyboard;
static Mouse mouse;
static bool is_mouse_exist;


static Axel_state_code wait_input_buffer_empty(void);
static Axel_state_code wait_output_buffer_full(void);
static Axel_state_code wait_response(enum ps2_constants);
static Axel_state_code write_ctrl_cmd(uint8_t const);
static Axel_state_code write_data_cmd(uint8_t const);
static Axel_state_code read_data(uint8_t*);


Axel_state_code init_keyboard(void) {
    uint8_t response = 0;
    Keyboard_config_byte kcb;

    keyboard.enable_caps_lock   = false;
    keyboard.enable_num_lock    = false;
    keyboard.enable_scroll_lock = false;
    keyboard.enable_keybord     = false;
    aqueue_init(&keyboard.aqueue, sizeof(uint8_t), KEYBOARD_BUFFER_SIZE, NULL);
    axel_s.keyboard = &keyboard;

    /* Disable keyboard and mouse. */
    FAILED_RETURN(write_ctrl_cmd(KEYBOARD_CTRL_CMD_DISABLE));
    FAILED_RETURN(write_ctrl_cmd(KEYBOARD_CTRL_CMD_DISABLE_MOUSE));

    /* Flush output buffer. */
    while (read_data(&response) == AXEL_SUCCESS) {}

    /* Read already configuration byte. */
    FAILED_RETURN(write_ctrl_cmd(KEYBOARD_CTRL_CMD_READ));
    FAILED_RETURN(read_data(&kcb.bit_expr));
    kcb.keyboard_interrupt_enable = 0;
    kcb.mouse_interrupt_enable    = 0;
    kcb.scancode_trans_enable     = 0;
    kcb.mouse_disable             = 1;
    kcb.keyboard_disable          = 1;

    /* Write modified value for flushing. */
    FAILED_RETURN(write_ctrl_cmd(KEYBOARD_CTRL_CMD_WRITE));
    FAILED_RETURN(write_data_cmd(kcb.bit_expr));

    /* Perform self test. */
    FAILED_RETURN(write_ctrl_cmd(KEYBOARD_CTRL_CMD_SELF_TEST));
    FAILED_RETURN(wait_response(KEYBOARD_SELF_TEST_SUCCESS));

    /* Check mouse */
    if (kcb.mouse_disable == 1) {
        is_mouse_exist = false;

        /* If mouse is disable,  We will check mouse. */
        FAILED_RETURN(write_ctrl_cmd(KEYBOARD_CTRL_CMD_ENABLE_MOUSE));
        FAILED_RETURN(write_ctrl_cmd(KEYBOARD_CTRL_CMD_READ));
        FAILED_RETURN(read_data(&response));
        if (((Keyboard_config_byte)response).mouse_disable == 0) {
            /*
             * mouse is enable.
             * So, this machine has mouse.
             */
            FAILED_RETURN(write_ctrl_cmd(KEYBOARD_CTRL_CMD_DISABLE));
            is_mouse_exist = true;
        }
    } else {
        is_mouse_exist = true;
    }

    /* Perform interface test. */
    FAILED_RETURN(write_ctrl_cmd(KEYBOARD_CTRL_CMD_INTERFACE_TEST));
    FAILED_RETURN(wait_response(KEYBOARD_INTERFACE_TEST_SUCCESS));
    if (is_mouse_exist == true) {
        FAILED_RETURN(write_ctrl_cmd(KEYBOARD_CTRL_CMD_MOUSE_TEST));
        FAILED_RETURN(wait_response(MOUSE_TEST_SUCCESS));
    }

    /* Enable keyboard. */
    FAILED_RETURN(write_ctrl_cmd(KEYBOARD_CTRL_CMD_ENABLE));

    /* Set configuration byte to enable some functions. */
    FAILED_RETURN(write_ctrl_cmd(KEYBOARD_CTRL_CMD_READ));
    FAILED_RETURN(read_data(&kcb.bit_expr));
    kcb.keyboard_interrupt_enable = 1;
    kcb.mouse_interrupt_enable    = 0;
    kcb.scancode_trans_enable     = 0;
    kcb.keyboard_disable          = 0;
    kcb.mouse_disable             = 1;
    FAILED_RETURN(write_ctrl_cmd(KEYBOARD_CTRL_CMD_WRITE));
    FAILED_RETURN(write_data_cmd(kcb.bit_expr));

    /* Reset */
    FAILED_RETURN(write_ctrl_cmd(PS2_RESET_CMD));

    enable_interrupt(PIC_IMR_MASK_IRQ01);

    keyboard.enable_keybord = true;

    return update_keyboard_led();
}


void interrupt_keybord(uint32_t* esp) {
    uint8_t keycode;
    if (AXEL_SUCCESS == read_data(&keycode)) {
        aqueue_insert(&keyboard.aqueue, &keycode);
    }
    send_done_interrupt_master();
}


Axel_state_code init_mouse(void) {
    mouse.enable_mouse = false;
    mouse.buttons.bit_expr = MOUSE_BUTTON_NONE;
    mouse.phase = 0;
    mouse.is_pos_update = false;
    memset(mouse.packets, 0, 4);
    clear_point2d(&mouse.pos);
    aqueue_init(&mouse.aqueue, sizeof(uint8_t), MOUSE_BUFFER_SIZE, NULL);
    axel_s.mouse = &mouse;

    if (is_mouse_exist == false) {
        return AXEL_FAILED;
    }
    Keyboard_config_byte kcb;

    /* Enable mouse */
    FAILED_RETURN(write_ctrl_cmd(KEYBOARD_CTRL_CMD_ENABLE_MOUSE));

    /* Set configuration byte to enable some functions. */
    FAILED_RETURN(write_ctrl_cmd(KEYBOARD_CTRL_CMD_READ));
    FAILED_RETURN(read_data(&kcb.bit_expr));
    kcb.mouse_interrupt_enable = 1;
    kcb.mouse_disable          = 0;
    FAILED_RETURN(write_ctrl_cmd(KEYBOARD_CTRL_CMD_WRITE));
    FAILED_RETURN(write_data_cmd(kcb.bit_expr));

    FAILED_RETURN(write_ctrl_cmd(KEYBOARD_CTRL_CMD_WRITE_MOUSE));
    FAILED_RETURN(write_data_cmd(MOUSE_COMMAND_BYTE_ENABLE_STREAM));
    FAILED_RETURN(wait_response(PS2_ACKNOWLEDGE));

    enable_interrupt(PIC_IMR_MASK_IRQ12);

    mouse.enable_mouse = true;

    return AXEL_SUCCESS;
}


void interrupt_mouse(void) {
    send_done_interrupt_slave();
    send_done_interrupt_master();

    uint8_t mdata;
    if (AXEL_SUCCESS == read_data(&mdata)) {
        aqueue_insert(&mouse.aqueue, &mdata);
    }
}


Axel_state_code update_keyboard_led(void) {
    uint8_t state = 0;
    state |= ((keyboard.enable_scroll_lock == true) ? KEYBOARD_LED_SCROLL : 0);
    state |= ((keyboard.enable_num_lock == true) ? KEYBOARD_LED_NUM : 0);
    state |= ((keyboard.enable_caps_lock == true) ? KEYBOARD_LED_CAPS : 0);

    FAILED_RETURN(write_data_cmd(KEYBOARD_DATA_CMD_LED));
    FAILED_RETURN(wait_response(PS2_ACKNOWLEDGE));
    FAILED_RETURN(write_data_cmd(state));
    FAILED_RETURN(wait_response(PS2_ACKNOWLEDGE));

    return AXEL_SUCCESS;
}


static inline Keyboard_status_flags read_status(void) {
    return (Keyboard_status_flags){.bit_expr = io_in8(KEYBOARD_STATUS_PORT)};
}


static inline Axel_state_code wait_state(uint8_t const mask, uint8_t const state) {
    for (uint8_t i = 0; i < UINT8_MAX; i++) {
        if ((read_status().bit_expr & mask) == state) {
            /* Wait until current state becomes argument state. */
            return AXEL_SUCCESS;
        }
    }

    return AXEL_FAILED;
}


static inline Axel_state_code wait_input_buffer_empty(void) {
    return wait_state(KEYBOARD_STATUS_INPUT_BUFFER_FULL, 0);
}


static inline Axel_state_code wait_output_buffer_full(void) {
    return wait_state(KEYBOARD_STATUS_OUTPUT_BUFFER_FULL, KEYBOARD_STATUS_OUTPUT_BUFFER_FULL);
}


static inline Axel_state_code read_data(uint8_t* data) {
    if (wait_output_buffer_full() != AXEL_SUCCESS) {
        return AXEL_FAILED;
    }

    *data = io_in8(KEYBOARD_DATA_PORT);

    return AXEL_SUCCESS;
}


static inline Axel_state_code wait_response(enum ps2_constants res_code) {
    uint8_t response = 0;

    for (uint8_t i = 0; i < UINT8_MAX; i++) {
        read_data(&response);
        if (response ==  res_code) {
            return AXEL_SUCCESS;
        }
    }

    return AXEL_FAILED;
}


static inline Axel_state_code write_command(uint8_t const port, uint8_t const cmd) {
    /*
     * Write command to keyboard.
     * When input buffer is empty.
     */
    if (wait_input_buffer_empty() == AXEL_FAILED) {
        return AXEL_FAILED;
    }
    io_out8(port, cmd);

    return AXEL_SUCCESS;
}


static inline Axel_state_code write_ctrl_cmd(uint8_t const cmd) {
    return write_command(KEYBOARD_COMMAND_PORT, cmd);
}


static inline Axel_state_code write_data_cmd(uint8_t const cmd) {
    return write_command(KEYBOARD_DATA_PORT, cmd);
}
