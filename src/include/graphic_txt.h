/************************************************************
 * File: include/graphic_txt.h
 * Description: TextMode Graphic Header.
 ************************************************************/

#ifndef GRAPHIC_TXT
#define GRAPHIC_TXT

#include <stdint.h>

enum Textmode_gpaphic_constants {
    TEXTMODE_VRAM_ADDR          = 0x000B8000,
    TEXTMODE_ATTR_FORE_COLOR_B  = 0x0100,
    TEXTMODE_ATTR_FORE_COLOR_G  = 0x0200,
    TEXTMODE_ATTR_FORE_COLOR_R  = 0x0400,
    TEXTMODE_ATTR_FBRI_OR_FONT  = 0x0800,
    TEXTMODE_ATTR_BACK_COLOR_B  = 0x1000,
    TEXTMODE_ATTR_BACK_COLOR_G  = 0x2000,
    TEXTMODE_ATTR_BACK_COLOR_R  = 0x4000,
    TEXTMODE_ATTR_BBRI_OR_BLNK  = 0x8000,
    TEXTMODE_DISPLAY_MAX_Y      = 25,
    TEXTMODE_DISPLAY_MAX_X      = 80,
    TEXTMODE_DISPLAY_MAX_XY     = (TEXTMODE_DISPLAY_MAX_X * TEXTMODE_DISPLAY_MAX_Y),
};

extern int putchar(int);
extern const char* puts(const char*);
extern void clean_screen(void);
extern void printf(const char*, ...);

#endif
