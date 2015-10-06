OUTPUT_FORMAT("binary")

    /* loader2 entry point. */
    ENTRY(loader2)
SECTIONS {

    /* loader2 is loaded 0x7C00 by loader1. */
    . = 0x7C00;

    .text : {
        *(.text*)
    }


    .data : ALIGN(0x1000) {
        *(.data*)
        *(.rodata*)
    }


    .bss : ALIGN(0x1000) {
        LD_LOADER2_BSS_BEGIN = .;
        *(.bss*)
        *(COMMON)
        LD_LOADER2_BSS_END = .;
    }
}
