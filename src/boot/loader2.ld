OUTPUT_FORMAT("binary")

SECTIONS {
    /* loader2 entry point. */
    ENTRY(loader2)

    /* loader2 is loaded 0x7C00 by loader1. */
    . = 0x7C00;

    .text : {
        *(.text*)
    }


    .data : {
        *(.data*)
        *(.rodata*)
    }


    .bss : {
        LD_LOADER2_BSS_BEGIN = .;
        *(.bss*)
        *(COMMON)
        LD_LOADER2_BSS_END = .;
    }
}
