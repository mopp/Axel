/************************************************************
 * File: include/macros.h
 * Description: This file contains utility macros.
 ************************************************************/

#ifndef MACROS_H
#define MACROS_H


#define _STR(x)             #x
#define _STR2(x)            _STR(x)
#define CURRENT_LINE_STR    _STR2(__LINE__)
#define HERE_STRING         __FILE__  ":" CURRENT_LINE_STR " "


#endif
