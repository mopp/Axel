/************************************************************
 * File: graphic_txt.c
 * Description: Some output functions for text mode graphic.
 *  And global function must be have "txt" suffix.
 ************************************************************/

#include <graphic_txt.h>
#include <point.h>
#include <stdarg.h>
#include <string.h>


/* text mode video ram */
static volatile uint16_t* vram = (uint16_t*)TEXTMODE_VRAM_ADDR;

/* current position in display. */
static Point2d pos;

/* background and foreground color attribution. */
static uint16_t disp_attribute = (TEXTMODE_ATTR_BACK_COLOR_B | TEXTMODE_ATTR_FORE_COLOR_G);

static uint16_t const SPACE_CHAR = 0x0020;


static inline void newline(void);
static void scroll_up_screen(void);
static inline uint16_t get_one_pixel(uint16_t);



void clean_screen(void) {
    for (uint16_t i = 0; i < TEXTMODE_DISPLAY_MAX_XY; ++i) {
        vram[i] = get_one_pixel(SPACE_CHAR);
    }

    /* clear */
    set_point2d(&pos, 0, 0);
}


int putchar(int c) {
    if (c == '\n' || c == '\r') {
        newline();
        return c;
    }

    vram[pos.x + pos.y * TEXTMODE_DISPLAY_MAX_X] = get_one_pixel(c);

    ++pos.x;
    if (TEXTMODE_DISPLAY_MAX_X <= pos.x) {
        scroll_up_screen();
    }

    return c;
}


const char* puts(const char* str) {
    while (*str != '\0') {
        putchar(*str++);
    }

    return str;
}


void printf(const char* format, ...) {
    va_list args;

    va_start(args, format);

    for (const char* c = format; *c != '\0'; ++c) {
        if (*c != '%') {
            putchar(*c);
            continue;
        }

        ++c;
        switch (*c) {
            case '\n':
                putchar('\n');
            case 's':
                puts(va_arg(args, const char*));
                break;
            case 'd':
            case 'x': {  // add local scope
                /* INT_MAX = +32767 なので最大の5桁以上のバッファを確保 */
                char buf[10];
                puts(itoa(buf, *c, va_arg(args, int)));
                break;
            }
        }
    }

    va_end(args);
}


static inline uint16_t get_one_pixel(uint16_t c) {
    /* 上1バイトが属性 */
    /* 下1バイトが文字 */
    return (disp_attribute | c);
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
