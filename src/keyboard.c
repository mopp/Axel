/**
 * @file keyboard.c
 * @brief related keyboard functions.
 * @author mopp
 * @version 0.1
 * @date 2014-04-11
 */
#include <keyboard.h>
#include <point.h>
#include <asm_functions.h>
#include <graphic.h>
#include <kernel.h>
#include <interrupt.h>


static Keyboard keyboard;


static Axel_state_code update_led(void);
static uint8_t read_register(uint8_t const);
static Keyboard_controle_status read_status_reg(void);
static uint8_t read_data_reg(void);
static Axel_state_code write_command(uint8_t const, uint8_t const);
static Axel_state_code write_control_command(uint8_t const);
static Axel_state_code write_data_command(uint8_t const);
static Axel_state_code wait_state(uint8_t const, uint8_t const);
static Axel_state_code wait_input_buffer_empty(void);
static Axel_state_code wait_output_buffer_full(void);



Axel_state_code init_keyboard(void) {
    keyboard.enable_calps_lock = false;
    keyboard.enable_num_lock = false;
    keyboard.enable_scroll_lock = false;
    keyboard.enable_keybord = true;
    aqueue_init(&keyboard.aqueue, sizeof(uint8_t), KEYBOARD_BUFFER_SIZE, NULL);

    wait_input_buffer_empty();

    /* do self test */
    if (write_control_command(KEYBOARD_CONTROL_COMMAND_SELF_TEST) == AXEL_FAILED) {
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
    if (write_control_command(KEYBOARD_CONTROL_COMMAND_WRITE) == AXEL_FAILED) {
        return AXEL_FAILED;
    }
    wait_input_buffer_empty();
    write_data_command(KEYBOARD_COMMAND_BYTE_KIE | KEYBOARD_COMMAND_BYTE_MIE | KEYBOARD_COMMAND_BYTE_SYSF | KEYBOARD_COMMAND_BYTE_XLATE);

    if (update_led() == AXEL_FAILED) {
       return AXEL_FAILED;
    }

    enable_interrupt(PIC_IMR_MASK_IRQ01);

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
    }

    send_done_interrupt_master();
}


static Axel_state_code update_led(void) {
    uint8_t state = 0;
    state |= keyboard.enable_calps_lock == true ? KEYBOARD_LED_CAPS : 0;
    state |= keyboard.enable_num_lock == true ? KEYBOARD_LED_NUM : 0;
    state |= keyboard.enable_scroll_lock == true ? KEYBOARD_LED_SCROLL : 0;

    if (write_data_command(KEYBOARD_DATA_COMMAND_LED) == AXEL_FAILED && write_data_command(state) == AXEL_FAILED) {
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


// static inline Axel_state_code wait_output_buffer_empty(void) {
//     return wait_state(KEYBOARD_STATUS_OUTPUT_BUFFER_FULL, 0);
// }
