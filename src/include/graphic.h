/************************************************************
 * File: include/graphic.h
 * Description: graphic code header.
 ************************************************************/

#ifndef GRAPHIC_H
#define GRAPHIC_H


#include <flag.h>
#include <rgb8.h>
#include <state_code.h>
#include <stdarg.h>
#include <vbe.h>

extern Axel_state_code init_graphic(Vbe_info_block const* const, Vbe_mode_info_block const* const);
extern void clean_screen(void);
extern int putchar(int);
extern const char* puts(const char*);

#ifdef TEXT_MODE
// TODO: define color presets
#else
// TODO: define color presets
#endif



#endif
