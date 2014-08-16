/************************************************************
 * File: include/asm_functions.h
 * Description: It provides prototype for
 *              assembly only fanctions
 ************************************************************/

#ifndef _ASM_FUNCTIONS_H_
#define _ASM_FUNCTIONS_H_



#include <stdint.h>
#include <paging.h>

extern void io_cli(void);
extern void io_sti(void);
extern void io_hlt(void);

extern uint8_t io_in8(uint16_t);
extern uint16_t io_in16(uint16_t);
extern uint32_t io_in32(uint16_t);

extern void io_out8(uint16_t, uint8_t);
extern void io_out16(uint16_t, uint16_t);
extern void io_out32(uint16_t, uint32_t);

extern void load_gdtr(uint32_t, uint32_t);
extern void load_idtr(uint32_t, uint32_t);

extern void turn_off_4MB_paging(void);
extern void flush_tlb(uintptr_t);
extern Page_directory_table get_cpu_pdt(void);
extern void set_cpu_pdt(Page_directory_table);
extern void flush_tlb_all(void);
extern void turn_on_pge(void);
extern void turn_off_pge(void);

extern uint16_t get_task_register(void);
extern void set_task_register(uint16_t);



#endif
