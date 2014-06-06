/************************************************************
 * File: include/macros.h
 * Description: This file contains utility macros.
 ************************************************************/

#ifndef _MACROS_H_
#define _MACROS_H_


#include <stdint.h>

#define QUOTE(x) #x
#define TO_STR(x) QUOTE(x)
#define CURRENT_LINE_STR TO_STR(__LINE__)
#define HERE_STRING (__FILE__ " " CURRENT_LINE_STR ":")

#define IS_ENABLE(x) (((x) != 0) ? 1 : 0)

#define ARRAY_SIZE_OF(a) (sizeof(a) / sizeof(a[0]))
#define ARRAY_LAST_ELEM(a) ((a) + (ARRAY_SIZE_OF(a) - (size_t)1))

#define ECAST_UINT8(n) ((uint8_t)(0xffu & n))
#define ECAST_UINT16(n) ((uint16_t)(0xffffu & n))
#define ECAST_UINT32(n) ((uint32_t)(0xffffffffu & n))

#define BIT(x) (1 << (x))

#define PRINTF_DEC_FORMAT(x) _Generic((x),  \
    char: "%c",                             \
    signed char: "%hhd",                    \
    unsigned char: "%hhu",                  \
    signed short: "%hd",                    \
    unsigned short: "%hu",                  \
    signed int: "%d",                       \
    unsigned int: "%u",                     \
    long int: "%ld",                        \
    unsigned long int: "%lu",               \
    long long int: "%lld",                  \
    unsigned long long int: "%llu",         \
    float: "%f",                            \
    double: "%f",                           \
    long double: "%Lf",                     \
    char *: "%s",                           \
    void *: "%p")

#include <stdio.h>
#define PRINT(x) printf(PRINTF_DEC_FORMAT(x), (x))
#define PRINTNL(x) PRINT(x), putchar('\n')


/* Get the name of a type */
#define TYPE_NAME_OF(x) _Generic((x),                   \
    _Bool                   : "_Bool",                  \
    char                    : "char",                   \
    signed char             : "signed char",            \
    unsigned char           : "unsigned char",          \
    short int               : "short int",              \
    unsigned short int      : "unsigned short int",     \
    int                     : "int",                    \
    unsigned int            : "unsigned int",           \
    long int                : "long int",               \
    unsigned long int       : "unsigned long int",      \
    long long int           : "long long int",          \
    unsigned long long int  : "unsigned long long int", \
    float                   : "float",                  \
    double                  : "double",                 \
    long double             : "long double",            \
    char*                   : "char*",                  \
    void*                   : "void*",                  \
    int*                    : "int*",                   \
    default                 : "unknown type")



#endif
