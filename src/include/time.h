/**
 * @file include/timer.h
 * @brief Timer implementation.
 * @author mopp
 * @version 0.1
 * @date 2014-10-30
 */

#ifndef _TIMER_H_
#define _TIMER_H_



#include <stdint.h>


extern void init_pit(void);
extern void wait(uint32_t);



#endif