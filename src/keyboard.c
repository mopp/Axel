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


static inline uint8_t read_keyboard_status(void) {
    return io_in8(KEYBOARD_CONTROL_PORT);
}


static inline Axel_state_code write_keyboard_command(uint8_t const cmd) {
    static uint8_t const RETRY_LIMIT = UINT8_MAX;
    uint8_t cnt = 0;

    while (cnt++ < RETRY_LIMIT) {
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


Axel_state_code set_keyboard_led(uint8_t led) {
    if ((led | KEYBOARD_LED_SCROLL | KEYBOARD_LED_NUM | KEYBOARD_LED_CAPS) == 0) {
        return AXEL_FAILED;
    }

    Axel_state_code state = 0;
    state = write_keyboard_command(KEYBOARD_DATA_COMMAND_LED);
    state |= write_keyboard_command(led);

    return state;
}


static inline void wait_write_ready(void) {
    while ((KEYBOARD_STATUS_IBF & io_in8(KEYBOARD_CONTROL_PORT)) != 0)
        ;
}


Axel_state_code init_keyboard(void) {
    /* do self test */
    wait_write_ready();
    if (write_keyboard_command(KEYBOARD_CONTROL_COMMAND_SELF_TEST) == AXEL_FAILED) {
        return AXEL_FAILED;
    }

    /* FIXME: add complete error checking. */
    wait_write_ready();
    read_keyboard_data();
    /* if (test_result == hoge) { */
    /* return AXEL_FAILED; */
    /* } */

    wait_write_ready();
    io_out8(KEYBOARD_CONTROL_PORT, KEYBOARD_CONTROL_COMMAND_WRITE_MODE);

    /* FIXME: use constant value. */
    wait_write_ready();
    io_out8(KEYBOARD_DATA_PORT, 0x47);

    io_out8(PIC0_IMR_DATA_PORT, io_in8(PIC0_IMR_DATA_PORT) & (0xFF & ~PIC_IMR_MASK_IRQ1));

    return AXEL_SUCCESS;
}
