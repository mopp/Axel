ENTRY(boot_kernel)
OUTPUT_FORMAT(elf32-i386)


SECTIONS {
    . = 0x100000;

    .multiboot_header : {
        *(.multiboot_header)
    }

    .boot_kernel : {
        *(.boot_kernel)
    }

    . += 0xC0000000;
    LD_KERNEL_START = .;

    .text : AT(ADDR(.text) - 0xC0000000) {
        _text_start = .;
        *(.text)
        *(.rodata*)
        _text_end = .;
    }

    .data ALIGN (0x1000) : AT(ADDR(.data) - 0xC0000000) {
        _data_start = .;
        *(.data)
        _data_end = .;
    }

    .bss : AT(ADDR(.bss) - 0xC0000000) {
        _bss_start = .;
        *(COMMON)
        *(.bss)
        _bss_end = .;
    }
    LD_KERNEL_END = .;

    LD_KERNEL_SIZE = LD_KERNEL_END - LD_KERNEL_START;
}
