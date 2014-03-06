/************************************************************
 * File: include/graphic.h
 * Description: graphic code header.
 ************************************************************/

#ifndef GRAPHIC_H
#define GRAPHIC_H


#include <flag.h>
#include <graphic_txt.h>
#include <graphic_vbe.h>
#include <stdarg.h>


extern void clean_screen(void);
extern int putchar(int);
extern const char* puts(const char*);
extern void printf(const char* , ...);

#endif
