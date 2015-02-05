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
#include <interrupt.h>


/*
 * Programmable Interval Timer
 * PIT Clock is 1193181.666...Hz
 * When counter value in PIT, IC fire interruption to IRQ0.
 * Interrupt frequency = PIT Clock frequency / Counter value.
 * "Counter value" = "PIT Clock frequency" / "Interrupt frequency".
 * "Interrupt frequency" is 100 Hz.
 * Then, "Counter value" = 1193182 / 100 = 11932(0x2E9C)
 * 1193182 / 0x2E9C(11932) = 100 Hz
 * 1 / frequency = sec
 * 1 / 100 = 10 ms
 * 1 sec / 10 ms = 100 interruption.
 */
enum PIT_constants {
    PIT_PORT_COUNTER0      = 0x40,
    PIT_PORT_COUNTER1      = 0x41,
    PIT_PORT_COUNTER2      = 0x42,
    PIT_PORT_CONTROL       = 0x43,
    PIT_ICW                = 0x34, /* Control word. */
    PIT_TIMER_1SEC_COUNT   = 100,
    PIT_COUNTER_VALUE_HIGH = 0x2E,
    PIT_COUNTER_VALUE_LOW  = 0x9C,
};


extern void init_pit(void);
extern void wait(uint32_t);
extern void interrupt_timer(Interrupt_frame*);



#endif
