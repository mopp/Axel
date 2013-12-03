/************************************************************
 * File: graphic_txt.c
 * Description: some output functions for TextMode Graphic.
 ************************************************************/

#include <graphic_txt.h>
#include <stdarg.h>
#include <string.h>


static inline void newline(void);

// text mode video ram
static volatile uint16_t* vram = (uint16_t*)TEXTMODE_VRAM_ADDR;
static int x_pos = 0, y_pos = 0;
static uint16_t disp_attribute = (TEXTMODE_ATTR_BACK_COLOR_B | TEXTMODE_ATTR_FORE_COLOR_G);


/* 画面を初期化する */
void clean_screen(void) {
    for (int i = 0; i < TEXTMODE_DISPLAY_MAX_XY; ++i) {
        /* 上1バイトが属性 */
        /* 下1バイトが文字 */
        vram[i] = (uint16_t)(disp_attribute | ' ');
    }

    /* clear */
    x_pos = 0;
    y_pos = 0;
}


/* 一文字を出力 */
int putchar(int c){
    if (c == '\n' || c == '\r') {
        newline();
        return c;
    }

    vram[x_pos + y_pos * TEXTMODE_DISPLAY_MAX_X] = (disp_attribute | c);

    ++x_pos;
    if (TEXTMODE_DISPLAY_MAX_X <= x_pos) {
        newline();
    }

    return c;
}

/* 文字列を出力 */
const char* puts(const char* str) {
    while (*str != '\0') {
        putchar(*str++);
    }

    return str;
}


/* フォーマットに従い文字列を出力 */
void printf(const char* format, ...) {
    va_list args;

    va_start(args, format);

    for (const char* c = format; *c!= '\0'; ++c) {
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
            case 'x':
                { // add local scope
                    /* INT_MAX = +32767 なので最大の5桁以上のバッファを確保 */
                    char buf[10];
                    puts(itoa(buf, *c, va_arg(args, int)));
                    break;
                }
        }
    }

    va_end(args);
}


/* 改行を行う */
static inline void newline(void) {
    /* 改行 */
    ++y_pos;
    if (TEXTMODE_DISPLAY_MAX_Y <= y_pos) {
        clean_screen();
    }

    x_pos = 0;

    return;
}
