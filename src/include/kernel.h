/**
 * @file include/kernel.h
 * @brief kernel header
 * @author mopp
 * @version 0.1
 * @date 2014-07-15
 */

#ifndef _KERNEL_H_
#define _KERNEL_H_



#include <ps2.h>


union segment_descriptor;
typedef union segment_descriptor Segment_descriptor;

/*
 * Axel operating system information structure.
 * This contains important global variable.
 */
struct axel_struct {
    Segment_descriptor* gdt;
    Keyboard* keyboard;
    Mouse* mouse;
};
typedef struct axel_struct Axel_struct;


extern Axel_struct axel_s;



#endif
