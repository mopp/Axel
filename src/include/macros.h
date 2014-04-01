/************************************************************
 * File: include/macros.h
 * Description: This file contains utility macros.
 ************************************************************/

#ifndef MACROS_H
#define MACROS_H


#include <stdint.h>

#define TO_STR(x) #x
#define TO_STR2(x) TO_STR(x)
#define CURRENT_LINE_STR TO_STR2(__LINE__)
#define HERE_STRING (__FILE__ " " CURRENT_LINE_STR ":")

#define TYPE_REP(type, i, init, cond) for (type i = (init); i < (cond); ++i)
#define REP(i, init, cond) TYPE_REP(int32_t, i, init, cond)
#define REP0(i, limit) FOR(i, 0, limit)

#define FOREACH(elem, set, next) for ((elem) = next(set); next(set) != NULL; (elem) = next(set))

#define IS_FLAG_NOT_ZERO(x) (((x) != 0) ? 1 : 0)

#define ARRAY_SIZE_OF(a) (sizeof(a) / sizeof(a[0]))
#define ARRAY_LAST_ELEM(a) ((a) + (ARRAY_SIZE_OF(a) - (size_t)1))


#endif
