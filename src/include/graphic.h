/************************************************************
 * File: include/graphic.h
 * Description: graphic code header.
 ************************************************************/

#ifndef GRAPHIC_H
#define GRAPHIC_H


#include <flag.h>

#ifdef TEXT_MODE
    #include <graphic_txt.h>
#else
    #include <graphic_vbe.h>
#endif


#endif
