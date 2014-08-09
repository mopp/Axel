/************************************************************
 * File: include/minunit.h
 * Description: This file contains custom MinUnit Test macros.
 *  from http://www.jera.com/techinfo/jtns/jtn002.html
 ************************************************************/

#ifndef _MIN_UNIT_H_
#define _MIN_UNIT_H_



#include <stdio.h>
#include <macros.h>


static int minunit_test_counter;


#define MIN_UNIT_ASSERT(msg, expr)                 \
    do {                                           \
        if ((expr) == 0) {                         \
            return "*ABORT* " HERE_STRING " " msg; \
        }                                          \
    } while (0)

#define MIN_UNIT_RUN(func_name)          \
    do {                                 \
        char const *msg = (func_name)(); \
        minunit_test_counter++;          \
        if (msg != NULL) {               \
            return msg;                  \
        }                                \
    } while (0)

#define MIN_UNIT_RUN_ALL(func_name)                               \
    do {                                                          \
        char const *result = (func_name)();                       \
                                                                  \
        if (result != NULL) {                                     \
            printf("%s\n", result);                               \
        } else {                                                  \
            printf("==ALL TESTS PASSED==\n");                     \
        }                                                         \
                                                                  \
        printf("The number of test: %d\n", minunit_test_counter); \
                                                                  \
        return result != NULL;                                    \
    } while (0)



#endif
