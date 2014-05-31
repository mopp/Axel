OUTPUT_FORMAT(elf32-i386)

LD_KERNEL_PHYSICAL_BASE_ADDR   = 0x100000;     /* It is base address before paging enable. */
LD_KERNEL_VIRTUAL_BASE_ADDR    = 0xC0000000;   /* It is base address after paging enable. */


SECTIONS {
    /* OS booting entry point. */
    ENTRY(boot_kernel)

    /*
     * Set current virtual address for paging.
     * Kernel is loaded at physical address 0x100000.
     * But We map kernel to higher address(0xC0000000~0xC0400000).
     * Therefor, add kernel physical address + virtual address in this.
     * NOTE that this mapping is 4MB mapping. NOT 4KB.
     */
    . = LD_KERNEL_PHYSICAL_BASE_ADDR + LD_KERNEL_VIRTUAL_BASE_ADDR;

    LD_KERNEL_START = .;

    /*
     * Entry point.
     * This is called by bootstraps loader.
     * NOTE that AT command indicates load memory address.
     * Kernel load address is NOT equal virtual address.
     * So, We subtract virtual base address.
     */
    .boot_kernel : AT(ADDR(.boot_kernel) - LD_KERNEL_VIRTUAL_BASE_ADDR) {
        *(.multiboot_header)
        *(.boot_kernel)
    }


    .text : AT(ADDR(.text) - LD_KERNEL_VIRTUAL_BASE_ADDR) {
        *(.text*)
    }


    .data : AT(ADDR(.data) - LD_KERNEL_VIRTUAL_BASE_ADDR) {
        *(.data*)
        *(.rodata*)
    }


    .bss : AT(ADDR(.bss) - LD_KERNEL_VIRTUAL_BASE_ADDR) {
        LD_KERNEL_BSS_START = .;
        *(.bss*)
        *(COMMON)
        LD_KERNEL_BSS_END = .;
        LD_KERNEL_BSS_SIZE = LD_KERNEL_BSS_END - LD_KERNEL_BSS_START;
    }


    LD_KERNEL_END = .;
    LD_KERNEL_SIZE = LD_KERNEL_END - LD_KERNEL_START;
}
