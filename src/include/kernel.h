/************************************************************
 * File: include/kernel.h
 * Description: kernel header
 *              TODO: Add comment
 ************************************************************/

#ifndef _KERNEL_H_
#define _KERNEL_H_


#include <ps2.h>


/*
 * Axel operating system information structure.
 * This contains important global variable.
 */
struct axel_struct {
    Keyboard* keyboard;
    Mouse* mouse;
};
typedef struct axel_struct Axel_struct;


extern Axel_struct axel_s;


#endif
