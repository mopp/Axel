/**
 * @file include/proc.h
 * @brief process header.
 * @author mopp
 * @version 0.1
 * @date 2014-07-15
 */

#ifndef _PROC_H_
#define _PROC_H_


#include <state_code.h>


Axel_state_code init_process(void);
void switch_context(void);


#endif
