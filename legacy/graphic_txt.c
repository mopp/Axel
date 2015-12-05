/************************************************************
 * File: graphic_txt.c
 * Description: Some output functions for text mode graphic.
 *  And global function must be have "txt" suffix.
 ************************************************************/

#include <graphic_txt.h>
#include <point.h>


/* text mode video ram */
static volatile uint16_t* vram = (uint16_t*)TEXTMODE_VRAM_ADDR;

/* current position in display. */
static Point2d pos;

/* background and foreground color attribution. */
static uint16_t disp_attribute = (TEXTMODE_ATTR_BACK_COLOR_B | TEXTMODE_ATTR_FORE_COLOR_G);

/* space character code. */
static uint8_t const SPACE_CHAR = 0x20;


static inline void newline(void);
static void scroll_up_screen(void);
static inline uint16_t get_one_pixel(uint8_t);



void clean_screen_txt(void) {
    for (uint16_t i = 0; i < TEXTMODE_DISPLAY_MAX_XY; ++i) {
        vram[i] = get_one_pixel(SPACE_CHAR);
    }

    /* clear */
    set_point2d(&pos, 0, 0);
}


int putchar_txt(int c) {
    if (c == '\n' || c == '\r') {
        newline();
        return c;
    }

    vram[pos.x + pos.y * TEXTMODE_DISPLAY_MAX_X] = get_one_pixel((uint8_t)c);

    ++pos.x;
    if (TEXTMODE_DISPLAY_MAX_X <= pos.x) {
        scroll_up_screen();
    }

    return c;
}


static inline uint16_t get_one_pixel(uint8_t c) {
    /* 上1バイトが属性 */
    /* 下1バイトが文字 */
    uint16_t t = 0x00FF & (uint16_t)c;
    return (disp_attribute | t);
}


static void scroll_up_screen(void) {
    /* shift up one line */
    for (uint16_t i = TEXTMODE_DISPLAY_MAX_X; i < TEXTMODE_DISPLAY_MAX_XY; ++i) {
        vram[i - TEXTMODE_DISPLAY_MAX_X] = vram[i];
    }

    /* clear tail line */
    for (uint16_t i = TEXTMODE_DISPLAY_MAX_X * (TEXTMODE_DISPLAY_MAX_Y - 1); i < TEXTMODE_DISPLAY_MAX_XY; ++i) {
        vram[i] = get_one_pixel(SPACE_CHAR);
    }

    --pos.y;
}


static inline void newline(void) {
    ++pos.y;
    if (TEXTMODE_DISPLAY_MAX_Y <= pos.y) {
        scroll_up_screen();
    }

    pos.x = 0;

    return;
}
