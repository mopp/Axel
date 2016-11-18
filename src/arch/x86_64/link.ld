/**
 * @brief Linker script to link kernel.
 * @author mopp
 * @version 0.2
 *
 *  Please refer LD documents for more information.
 *  https://sourceware.org/binutils/docs/ld/index.html
 */


/*
 * Set the output file format.
 * You can see the available formats by `objdump --info`.
 */
OUTPUT_FORMAT(elf64-x86-64)

/* Set kernel entry point. */
ENTRY(start_axel)


SECTIONS
{
    KERNEL_ADDR_PHYSICAL_BASE = 0x00100000;
    . = KERNEL_ADDR_PHYSICAL_BASE;

    /*
     * The kernel LMA (Load Memory Address) and VMA (Virtual Memory Address) is equal here in order to use jump instruction in the protect mode.
     */
    .32bit_text :
    {
        /*
         * If linker option `--gc-sections` was passed, linker eliminates unused sections (e.g., multiboot2).
         * However, multiboot header must not be removed and located at the head.
         * So, KEEP is used to keep these sections here.
         */
        KEEP(*(.multiboot2))
        *(.32bit_text)
    }

    /*
     * The kernel LMA and VMA is different each other from here.
     *  LMA = KERNEL_ADDR_PHYSICAL_BASE
     *  VMA = KERNEL_ADDR_PHYSICAL_BASE + KERNEL_ADDR_VIRTUAL_BASE
     * The kernel will be load at this physical address by a bootloader.
     * After that, the kernel will be mapped to the higher address.
     * You can confirm these information using the command `objdump -w -x`.
     */
    KERNEL_ADDR_VIRTUAL_BASE  = 0x40000000;
    . = KERNEL_ADDR_PHYSICAL_BASE + KERNEL_ADDR_VIRTUAL_BASE + SIZEOF(.32bit_text);

    /*
     * Each section will be aligned on 4096.
     * This alignment means the page size.
     */
    .text : AT(ADDR(.text) - KERNEL_ADDR_VIRTUAL_BASE)
    {
        *(.64bit_text)
        *(.text*)
    }

    .data : AT(ADDR(.data) - KERNEL_ADDR_VIRTUAL_BASE)
    {
        *(.data*)
    }

    .bss : AT(ADDR(.bss) - KERNEL_ADDR_VIRTUAL_BASE)
    {
        KERNEL_ADDR_BSS_BEGIN = .;
        *(.bss*)
        KERNEL_ADDR_BSS_END = .;
    }

    .rodata : AT(ADDR(.rodata) - KERNEL_ADDR_VIRTUAL_BASE)
    {
        *(.rodata*)
    }

    KERNEL_SIZE_BSS   = KERNEL_ADDR_BSS_END - KERNEL_ADDR_BSS_BEGIN;
    KERNEL_SIZE_TOTAL = . - (KERNEL_ADDR_PHYSICAL_BASE + KERNEL_ADDR_VIRTUAL_BASE);
}
