/**
 * @file syscall.c
 * @brief system call implementation.
 * @author mopp
 * @version 0.1
 * @date 2014-10-30
 */


#include <syscall.h>
#include <interrupt.h>
#include <utils.h>



void syscall_enter(Interrupt_frame* iframe) {
    printf("0x%x\n", iframe->eax);
}
