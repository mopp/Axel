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
#include <kernel.h>
#include <interrupt.h>
#include <string.h>


union keyboard_controle_status {
    struct {
        uint8_t output_buf_full : 1; /* If this is 1, data exists into output register. */
        uint8_t input_buf_full : 1;  /* If this is 1, data exists into input register. may be, controller executing. */
        uint8_t system_flag : 1;     /* This is changed by command result. */
        uint8_t cmd_data_flag : 1;   /* If this is 1, Last writting is command. otherwise, last writting is data */
        uint8_t lock_flag : 1;       /* If this is 0, keyboard is locked. otherwise, not locked. */
        uint8_t mouse_data_came : 1; /* If this is 1, there are mouse data at KEYBOARD_OUTPUT_PORT. otherwise, there are keyboard data. */
        uint8_t write_timeout : 1;
        uint8_t parity_error : 1;
    };
    uint8_t bit_expr;
};
typedef union keyboard_controle_status Keyboard_controle_status;
_Static_assert(sizeof(Keyboard_controle_status) == 1, "Keyboard_controle_status is NOT 1 byte.");


enum keyboard_constants {
    KEYBOARD_BUFFER_SIZE               = 128,  /* keyboard buffer size */
    MOUSE_BUFFER_SIZE                  = 128,  /* mouse buffer size */

    KEYBOARD_DATA_PORT                 = 0x60, /* Read: Output from keyboard register, Write: input to keyboard register */
    KEYBOARD_OUTPUT_PORT               = 0x60, /* for Read. */
    KEYBOARD_INPUT_PORT                = 0x60, /* for Write. */

    KEYBOARD_CTRL_PORT                 = 0x64, /* Read: Status register,  Write: Command register */
    KEYBOARD_STATUS_PORT               = 0x64, /* for Read. */
    KEYBOARD_COMMAND_PORT              = 0x64, /* for Write. */

    KEYBOARD_STATUS_OUTPUT_BUFFER_FULL = 0x01,
    KEYBOARD_STATUS_INPUT_BUFFER_FULL  = 0x02,
    KEYBOARD_STATUS_SYSTEM_FLAG        = 0x04,
    KEYBOARD_STATUS_CMD_DATA_FLAG      = 0x08,
    KEYBOARD_STATUS_LOCK_FLAG          = 0x10,
    KEYBOARD_STATUS_READ_TIMEOUT       = 0x20,
    KEYBOARD_STATUS_WRITE_TIMEOUT      = 0x40,
    KEYBOARD_STATUS_PARITY_ERROR       = 0x80,

    KEYBOARD_DATA_COMMAND_LED          = 0xED, /* Set LED for scroll, num, caps lock */
    KEYBOARD_DATA_COMMAND_ECHO         = 0xEE, /* return writting data(0xEE) to 0x60 */
    KEYBOARD_DATA_COMMAND_SCANCODE     = 0xF0, /* get or set scan code */
    KEYBOARD_DATA_COMMAND_KEYBOARD_ID  = 0xF2, /* get keyboard id, after writting this command, it takes at least 10ms. */

    KEYBOARD_LED_NUM                   = 0x01,
    KEYBOARD_LED_SCROLL                = 0x02,
    KEYBOARD_LED_CAPS                  = 0x04,

    KEYBOARD_CTRL_COMMAND_READ         = 0x20, /* read control command byte from keyboard controller. */
    KEYBOARD_CTRL_COMMAND_WRITE        = 0x60, /* write control command byte to keyboard controller. */
    KEYBOARD_CTRL_COMMAND_SELF_TEST    = 0xAA, /* do self test -> 0x55 is OK result, 0xFC is NG. */
    KEYBOARD_CTRL_COMMAND_READ_INPUT   = 0xC0, /* read data from input port, result is in 0x64. */
    KEYBOARD_CTRL_COMMAND_MOUSE_TEST   = 0xA9, /* do mouse test, result is in 0x60. */
    KEYBOARD_CTRL_COMMAND_WRITE_MOUSE  = 0xD4, /* write data to mouse. */

    KEYBOARD_COMMAND_BYTE_KIE          = 0x01, /* Keyboard interruput enable . */
    KEYBOARD_COMMAND_BYTE_MIE          = 0x02, /* Mouse interruput enable. */
    KEYBOARD_COMMAND_BYTE_SYSF         = 0x04, /* System flag. */
    KEYBOARD_COMMAND_BYTE_IGNLK        = 0x08, /* Ignore keyboard lock. */
    KEYBOARD_COMMAND_BYTE_KE           = 0x10, /* Keyboard enable. */
    KEYBOARD_COMMAND_BYTE_ME           = 0x20, /* Mouse enable. */
    KEYBOARD_COMMAND_BYTE_XLATE        = 0x40, /* Scan code translation enable. */

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
    MOUSE_TEST_SUCCESS                 = 0x00,

    PS2_ACKNOWLEDGE                    = 0xFA,
    PS2_RESEND                         = 0xFA,
};


enum mouse_constants {
    MOUSE_BUTTON_NONE= 0,
    MOUSE_BUTTON_RIGHT,
    MOUSE_BUTTON_LEFT,
    MOUSE_BUTTON_MIDDLE,
};



static Keyboard keyboard;
static Mouse mouse;


static uint8_t read_register(uint8_t const);
static uint8_t read_data_reg(void);
static Keyboard_controle_status read_status_reg(void);
static Axel_state_code update_led(void);
static Axel_state_code write_command(uint8_t const, uint8_t const);
static Axel_state_code write_control_command(uint8_t const);
static Axel_state_code write_data_command(uint8_t const);
static Axel_state_code wait_acknowledge(void);
static Axel_state_code wait_state(uint8_t const, uint8_t const);
static Axel_state_code wait_input_buffer_empty(void);
static Axel_state_code wait_output_buffer_full(void);
static Axel_state_code wait_output_buffer_empty(void);



Axel_state_code init_keyboard(void) {
    keyboard.enable_calps_lock = false;
    keyboard.enable_num_lock = false;
    keyboard.enable_scroll_lock = false;
    keyboard.enable_keybord = false;
    aqueue_init(&keyboard.aqueue, sizeof(uint8_t), KEYBOARD_BUFFER_SIZE, NULL);

    wait_input_buffer_empty();
    wait_output_buffer_empty();

    /* do self test */
    if (write_control_command(KEYBOARD_CTRL_COMMAND_SELF_TEST) == AXEL_FAILED) {
        return AXEL_FAILED;
    }

    wait_output_buffer_full();
    if (read_data_reg() == KEYBOARD_SELF_TEST_FAILED) {
        return AXEL_FAILED;
    }

    /*
     * Prepare writting control command byte.
     * Then, enable some setting.
     */
    wait_input_buffer_empty();
    if (write_control_command(KEYBOARD_CTRL_COMMAND_WRITE) == AXEL_FAILED) {
        return AXEL_FAILED;
    }
    wait_input_buffer_empty();
    write_data_command(KEYBOARD_COMMAND_BYTE_KIE | KEYBOARD_COMMAND_BYTE_MIE | KEYBOARD_COMMAND_BYTE_SYSF | KEYBOARD_COMMAND_BYTE_XLATE);

    if (update_led() == AXEL_FAILED) {
        return AXEL_FAILED;
    }

    enable_interrupt(PIC_IMR_MASK_IRQ01);

    keyboard.enable_keybord = true;
    return AXEL_SUCCESS;
}


void interrupt_keybord(uint32_t* esp) {
    static Point2d const start = {300, 500};
    static Point2d const sstart = {300 - 10 * 8, 500};
    static char buf[20] = {0};

    while (read_status_reg().output_buf_full == 1) {
        uint8_t key_code = read_data_reg();
        itoa(key_code & 0x3F, buf, 16);

        /* clear drawing area. */
        static RGB8 c;
        fill_rectangle(&start, &make_point2d(300 + 20, 500 + 13), set_rgb_by_color(&c, 0x3A6EA5));

        /* draw key code */
        puts_ascii_font("Key code: ", &sstart);
        puts_ascii_font(buf, &start);

        aqueue_insert(&keyboard.aqueue, &key_code);
        if (aqueue_is_full(&keyboard.aqueue) == true) {
            while (aqueue_is_empty(&keyboard.aqueue) == false) {
                aqueue_delete_first(&keyboard.aqueue);
            }
        }
    }

    send_done_interrupt_master();
}


Axel_state_code init_mouse(void) {
    mouse.enable_mouse = false;
    mouse.button = MOUSE_BUTTON_NONE;
    mouse.phase = 0;
    memset(mouse.packets, 0, 4);
    clear_point2d(&mouse.pos);
    aqueue_init(&mouse.aqueue, sizeof(uint8_t), KEYBOARD_BUFFER_SIZE, NULL);

    wait_input_buffer_empty();

    /* do self test */
    wait_input_buffer_empty();
    if (write_control_command(KEYBOARD_CTRL_COMMAND_MOUSE_TEST) == AXEL_FAILED) {
        return AXEL_FAILED;
    }
    wait_output_buffer_full();
    if (read_data_reg() != MOUSE_TEST_SUCCESS) {
        return AXEL_FAILED;
    }

    /*
     * Prepare writting control command byte.
     * Then, enable mouse data stream.
     */
    wait_input_buffer_empty();
    if (write_control_command(KEYBOARD_CTRL_COMMAND_WRITE_MOUSE) == AXEL_FAILED) {
        return AXEL_FAILED;
    }
    wait_input_buffer_empty();
    write_data_command(MOUSE_COMMAND_BYTE_ENABLE_STREAM);

    if (wait_acknowledge() == AXEL_FAILED) {
        return AXEL_FAILED;
    }


    if (update_led() == AXEL_FAILED) {
        return AXEL_FAILED;
    }

    enable_interrupt(PIC_IMR_MASK_IRQ12);

    mouse.enable_mouse = true;
    return AXEL_SUCCESS;
}


void interrupt_mouse(void) {
    uint8_t mdata= read_data_reg();
    aqueue_insert(&mouse.aqueue, &mdata);

    send_done_interrupt_slave();
    send_done_interrupt_master();
}


static Axel_state_code update_led(void) {
    uint8_t state = 0;
    state |= keyboard.enable_calps_lock == true ? KEYBOARD_LED_CAPS : 0;
    state |= keyboard.enable_num_lock == true ? KEYBOARD_LED_NUM : 0;
    state |= keyboard.enable_scroll_lock == true ? KEYBOARD_LED_SCROLL : 0;

    if (write_data_command(KEYBOARD_DATA_COMMAND_LED) == AXEL_FAILED) {
        return AXEL_FAILED;
    }

    if (wait_acknowledge() == AXEL_FAILED) {
        return AXEL_FAILED;
    }

    if (write_data_command(state) == AXEL_FAILED) {
        return AXEL_FAILED;
    }

    if (wait_acknowledge() == AXEL_FAILED) {
        return AXEL_FAILED;
    }

    return AXEL_SUCCESS;
}


static inline uint8_t read_register(uint8_t const port) {
    return io_in8(port);
}


static inline Keyboard_controle_status read_status_reg(void) {
    return (Keyboard_controle_status){.bit_expr = read_register(KEYBOARD_STATUS_PORT)};
}


static inline uint8_t read_data_reg(void) {
    return read_register(KEYBOARD_OUTPUT_PORT);
}


static inline Axel_state_code write_command(uint8_t const port, uint8_t const cmd) {
    uint8_t cnt = 0;

    while (cnt++ < UINT8_MAX) {
        if (read_status_reg().input_buf_full == 0) {
            /*
             * Write command to keyboard.
             * When input buffer is empty.
             */
            io_out8(port, cmd);
            return AXEL_SUCCESS;
        }
    }

    return AXEL_FAILED;
}


static inline Axel_state_code write_control_command(uint8_t const cmd) {
    return write_command(KEYBOARD_COMMAND_PORT, cmd);
}


static inline Axel_state_code write_data_command(uint8_t const cmd) {
    return write_command(KEYBOARD_OUTPUT_PORT, cmd);
}


static inline Axel_state_code wait_acknowledge(void) {
    for (uint16_t i = 0; i < UINT16_MAX; i++) {
        if (read_data_reg() == PS2_ACKNOWLEDGE) {
            /* Wait until receive ack signal. */
            return AXEL_SUCCESS;
        }
    }

    return AXEL_FAILED;
}


static inline Axel_state_code wait_state(uint8_t const mask, uint8_t const state) {
    for (uint16_t i = 0; i < UINT16_MAX; i++) {
        if ((read_status_reg().bit_expr & mask) == state) {
            /* Wait until current state becomes argument state. */
            return AXEL_SUCCESS;
        }
    }

    return AXEL_FAILED;
}


// static inline Axel_state_code wait_input_buffer_full(void) {
//     return wait_state(KEYBOARD_STATUS_INPUT_BUFFER_FULL, KEYBOARD_STATUS_INPUT_BUFFER_FULL);
// }


static inline Axel_state_code wait_input_buffer_empty(void) {
    return wait_state(KEYBOARD_STATUS_INPUT_BUFFER_FULL, 0);
}


static inline Axel_state_code wait_output_buffer_full(void) {
    return wait_state(KEYBOARD_STATUS_OUTPUT_BUFFER_FULL, KEYBOARD_STATUS_OUTPUT_BUFFER_FULL);
}


static inline Axel_state_code wait_output_buffer_empty(void) {
    return wait_state(KEYBOARD_STATUS_OUTPUT_BUFFER_FULL, 0);
}
