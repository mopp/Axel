/************************************************************
 * File: include/minunit.h
 * Description: This file contains custom MinUnit Test macros.
 *  from http://www.jera.com/techinfo/jtns/jtn002.html
 ************************************************************/

#ifndef MINUNIT_H
#define MINUNIT_H


#include <macros.h>

static uint32_t mu_all_tests_num;

#define MU_ASSERT(err_msg, exp)             \
    do {                                    \
        if (0 == (exp)) {                   \
            return HERE_STRING " " err_msg; \
        }                                   \
    } while (0)


#define MU_RUN_TEST(func_name)               \
    do {                                     \
        char const *const msg = func_name(); \
        ++mu_all_tests_num;                  \
        if (msg != 0) {                      \
            return msg;                      \
        }                                    \
    } while (0)


#endif
