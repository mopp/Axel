/************************************************************
 * File: include/asm_functions.h
 * Description: It provides prototype for
 *              assembly only fanctions
 ************************************************************/

#ifndef ASM_FUNCTIONS_H
#define ASM_FUNCTIONS_H

#include <stdint.h>

extern void io_cli(void);
extern void io_sti(void);
extern void io_hlt(void);

extern uint8_t  io_in8 (uint16_t);
extern uint16_t io_in16(uint16_t);
extern uint32_t io_in32(uint16_t);

extern void io_out8 (uint16_t, uint8_t );
extern void io_out16(uint16_t, uint16_t);
extern void io_out32(uint16_t, uint32_t);

extern void load_gdtr(int, int);
extern void load_idtr(int, int);
extern void change_segment_selectors(int);

#endif
