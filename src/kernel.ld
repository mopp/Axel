OUTPUT_FORMAT(elf32-i386)

LD_KERNEL_PHYSICAL_BASE_ADDR   = 0x100000;     /* It is base address before paging enable. */
LD_KERNEL_VIRTUAL_BASE_ADDR    = 0xC0000000;   /* It is base address after paging enable. */


SECTIONS {
    /* OS booting entry point. */
    ENTRY(boot_kernel)

    /*
     * Set kernel physical base address in current address.
     * Because bootstraps loader will load kernel at 0x100000
     * And this area is exexuted before enable paging.
     */
    . = LD_KERNEL_PHYSICAL_BASE_ADDR;

    /* This is required by multiboot specification. */
    .multiboot_header : {
        *(.multiboot_header)
    }

    /*
     * Entry point.
     * This is called by bootstraps loader.
     */
    .boot_kernel : {
        *(.boot_kernel)
    }

    /*
     * Paging is enable with the address below.
     * Therefor, add kernel virtual base address in this.
     */
    . += LD_KERNEL_VIRTUAL_BASE_ADDR;

    LD_KERNEL_START = .;

    .text ALIGN(0x1000) : AT(ADDR(.text) - LD_KERNEL_VIRTUAL_BASE_ADDR) {
        *(.text*)
    }

    .data : AT(ADDR(.data) - LD_KERNEL_VIRTUAL_BASE_ADDR) {
        *(.data*)
        *(.rodata*)
    }

    .bss : AT(ADDR(.bss) - LD_KERNEL_VIRTUAL_BASE_ADDR) {
        *(.bss*)
        *(COMMON)
    }

    LD_KERNEL_END = .;

    LD_KERNEL_SIZE = LD_KERNEL_END - LD_KERNEL_START;
}
