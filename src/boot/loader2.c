/**
 * @file loader2.c
 * @brief Second OS boot loader
 * @author mopp
 * @version 0.1
 * @date 2015-02-03
 */
#include <stdint.h>

_Noreturn void loader2(void) {
    char* vram = (char*)0xA0000;
    while (1) {
        if ((uintptr_t)vram == 0xB0000) {
            vram = (char*)0xA0000;
        }
        *vram = (char)0x03;
        vram++;
    }
}
