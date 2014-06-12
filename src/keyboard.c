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


static inline Keyboard_controle_status read_keyboard_status(void) {
    return (Keyboard_controle_status){.bit_expr = io_in8(KEYBOARD_CONTROL_PORT)};
}


static inline uint8_t read_keyboard_data(void) {
    return io_in8(KEYBOARD_DATA_PORT);
}


static inline Axel_state_code write_keyboard_command(uint8_t const cmd) {
    uint8_t cnt = 0;

    while (cnt++ < UINT8_MAX) {
        if (read_keyboard_status().input_buf_full == 0) {
            /*
             * Write control command to keyboard.
             * When input buffer is empty.
             */
            io_out8(KEYBOARD_CONTROL_PORT, cmd);
            return AXEL_SUCCESS;
        }
    }

    return AXEL_FAILED;
}


Axel_state_code set_keyboard_led(uint8_t led, bool enable) {
    // FIXME: store LED state.
    Axel_state_code state = AXEL_SUCCESS;
    state = write_keyboard_command(KEYBOARD_DATA_COMMAND_LED);
    state |= write_keyboard_command(led);

    return state;
}


static inline void wait_write_ready(void) {
    while (read_keyboard_status().input_buf_full == 1) {
        /* Wait until input buffer becomes empty. */
    }
}


/* FIXME: 整理しなおし */
Axel_state_code init_keyboard(void) {
    keyboard.enable_calps_lock = false;
    keyboard.enable_num_lock = false;
    keyboard.enable_scroll_lock = false;
    aqueue_init(&keyboard.aqueue, sizeof(uint8_t), KEYBOARD_BUFFER_SIZE, NULL);

    /* do self test */
    wait_write_ready();
    if (write_keyboard_command(KEYBOARD_CONTROL_COMMAND_SELF_TEST) == AXEL_FAILED) {
        return AXEL_FAILED;
    }

    wait_write_ready();
    read_keyboard_data();

    wait_write_ready();
    io_out8(KEYBOARD_CONTROL_PORT, KEYBOARD_CONTROL_COMMAND_WRITE_MODE); /* prepare writting control command. */
    wait_write_ready();
    io_out8(KEYBOARD_DATA_PORT, 0x47);

    io_out8(PIC0_IMR_DATA_PORT, io_in8(PIC0_IMR_DATA_PORT) & ~PIC_IMR_MASK_IRQ1);

    return AXEL_SUCCESS;
}


void interrupt_keybord(uint32_t* esp) {
    static Point2d const start = {300, 500};
    static Point2d const sstart = {300 - 10 * 8, 500};
    static char buf[20] = {0};

    while (read_keyboard_status().output_buf_full == 1) {
        uint8_t key_code = io_in8(KEYBOARD_DATA_PORT);
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
