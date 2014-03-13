/************************************************************
 * File: include/graphic.h
 * Description: graphic code header.
 ************************************************************/

#ifndef GRAPHIC_H
#define GRAPHIC_H


#include <flag.h>
#include <stdarg.h>
#include <rgb8.h>


// TODO: define color presets


extern void clean_screen(void);
extern int putchar(int);
extern const char* puts(const char*);
extern void printf(const char* , ...);


#endif
