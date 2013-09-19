/************************************************************
 * File: include/asm_functions.h
 * Description: It provides prototype for
 *              assembly only fanctions
 ************************************************************/

#ifndef ASM_FUNCTIONS_H
#define ASM_FUNCTIONS_H

extern void io_cli(void);
extern void io_sti(void);
extern void io_hlt(void);

extern int io_in8(int port);
extern int io_in16(int port);
extern int io_in32(int port);

extern void io_out8(int port, int data);
extern void io_out16(int port, int data);
extern void io_out32(int port, int data);

extern void load_gdtr(int, int);
extern void load_idtr(int, int);
extern void change_segment_selectors(int);

#endif
