/**
 * @brief Linker script to link OS binary.
 * @author mopp
 * @version 0.2
 * @date 2015-12-11
 *
 *  Please refer to documents in below link.
 *  https://sourceware.org/binutils/docs/ld/index.html
 */


/*
 * Set output file format.
 * You can see available list in your environment by `objdump --info`.
 */
OUTPUT_FORMAT(elf32-i386)

SECTIONS
{
    /*
     * Set kernel load address to location counter.
     * The kernel will be load at this address by bootloader.
     */
    . = 0x00100000;

    LD_KERNEL_BEGIN = .;

    /* Set OS entry point. */
    ENTRY(boot_kernel)

    /*
     * Each section will be aligned on a 4096.
     * This alignment means page size.
     */
    .text ALIGN(4096):
    {
        /*
         * If linker option `--gc-sections` is passed, linker eliminates unused section (e.g., never referenced section).
         * However, multiboot header should not be removed and located at head.
         * So, KEEP is used to keep these sections.
         */
        KEEP(*(.multiboot_header))
        KEEP(*(.boot_kernel))
        *(.text*)
    }

    .data ALIGN(4096):
    {
        *(.data*)
    }

    .bss ALIGN(4096):
    {
        LD_KERNEL_BSS_BEGIN = .;
        *(.bss*)
        LD_KERNEL_BSS_END = .;
    }

    .rodata ALIGN(4096):
    {
        *(.rodata*)
    }

    LD_KERNEL_END = .;
    LD_KERNEL_SIZE = LD_KERNEL_END - LD_KERNEL_BEGIN;
}
