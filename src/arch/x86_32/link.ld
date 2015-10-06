OUTPUT_FORMAT("elf32-i386")


SECTIONS {
    /* OS booting entry point. */
    ENTRY(boot_kernel)

    LD_KERNEL_START = .;

    . = 0x00100000;

    /*
     * Entry point.
     * This is called by bootstraps loader.
     * NOTE that AT command indicates load memory address.
     * Kernel load address is NOT equal virtual address.
     * So, We subtract virtual base address.
     */
    .boot_kernel BLOCK(4K) : ALIGN(4K)
    {
        *(.multiboot_header)
        *(.boot_kernel)
    }


    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.text*)
    }


    .data BLOCK(4K) : ALIGN(4K)
    {
        *(.data*)
        *(.rodata*)
    }


    .bss BLOCK(4K) : ALIGN(4K)
    {
        LD_KERNEL_BSS_START = .;
        *(.bss*)
        *(COMMON)
        LD_KERNEL_BSS_END = .;
    }


    LD_KERNEL_BSS_SIZE = LD_KERNEL_BSS_END - LD_KERNEL_BSS_START;
    LD_KERNEL_END = .;
    LD_KERNEL_SIZE = LD_KERNEL_END - LD_KERNEL_START;
}
