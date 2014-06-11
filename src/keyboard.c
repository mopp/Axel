/**
 * @file keyboard.c
 * @brief related keyboard functions.
 * @author mopp
 * @version 0.1
 * @date 2014-04-11
 */
#include <keyboard.h>
#include <asm_functions.h>
#include <kernel.h>

Keyboard keyboard;


static inline uint8_t read_keyboard_status(void) {
    return io_in8(KEYBOARD_CONTROL_PORT);
}


static inline Axel_state_code write_keyboard_command(uint8_t const cmd) {
    uint8_t cnt = 0;

    while (cnt++ < UINT8_MAX) {
        /* write control command to keyboard when input buffer full is empty. */
        if ((read_keyboard_status() & KEYBOARD_STATUS_IBF) == 0) {
            io_out8(KEYBOARD_CONTROL_PORT, cmd);
            return AXEL_SUCCESS;
        }
    }

    return AXEL_FAILED;
}


static inline uint8_t read_keyboard_data(void) {
    return io_in8(KEYBOARD_DATA_PORT);
}


Axel_state_code set_keyboard_led(uint8_t led, bool enable) {
    // FIXME: store LED state.
    Axel_state_code state = AXEL_SUCCESS;
    state = write_keyboard_command(KEYBOARD_DATA_COMMAND_LED);
    state |= write_keyboard_command(led);

    return state;
}


static inline void wait_write_ready(void) {
    while ((io_in8(KEYBOARD_CONTROL_PORT) & KEYBOARD_STATUS_IBF) != 0)
        ;
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
