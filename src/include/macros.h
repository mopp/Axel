/**
 * @file include/macros.h
 * @brief This file contains utility macros.
 * @author mopp
 * @version 0.1
 * @date 2014-10-13
 */


#ifndef _MACROS_H_
#define _MACROS_H_



#include <stdint.h>


#define QUOTE(x) #x
#define TO_STR(x) QUOTE(x)
#define CURRENT_LINE_STR TO_STR(__LINE__)
#define HERE_STRING (__FILE__ " " CURRENT_LINE_STR ":")

#define IS_ENABLE(x) (((x) != 0) ? 1 : 0)

#define ARRAY_SIZE_OF(a) (sizeof(a) / sizeof(a[0]))
#define ARRAY_LAST_ELEM(a) ((a) + (ARRAY_SIZE_OF(a) - (size_t)1))

#define ECAST_UINT8(n)   ((uint8_t)(n & 0x000000FFu))
#define ECAST_UINT16(n) ((uint16_t)(n & 0x0000FFFFu))
#define ECAST_UINT32(n) ((uint32_t)(n & 0xFFFFFFFFu))

#define INF_LOOP() do {} while (1)

#define DIRECTLY_WRITE(type, addr, val) (*((type *)(addr)) = (type)(val))
#define DIRECTLY_WRITE_STOP(type, addr, val) \
    do {                                     \
        (*((type *)(addr)) = (type)(val));   \
    } while (1)

#define BOCHS_MAGIC_BREAK() __asm__ volatile( "xchgw %bx, %bx");

#define BIT(x) (1u << (x))
#define PO2(x) BIT(x)

#define KB(x) (x >> 10)
#define MB(x) (x >> 20)
#define GB(x) (x >> 30)

#define TOGGLE_BOOLEAN(x) ((x) = ((x) == true ? false : true));

#define ALIGN_UP(n, align) ((n) + ((align) - 1u) & ~((align) - 1u))
#define ALIGN_DOWN(n, align) ((n) & ~((align) - 1u))



#endif
