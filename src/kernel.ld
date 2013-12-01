/* エントリポイントを設定 */
ENTRY(boot_kernel)

SECTIONS {
    /*
     * 以降のセグメントをカーネルの読み込み先である
     * 0x100000から開始するように
     * 現在のアドレスを設定
     */
    /* .  =  0x100000; */
    . = 1M;
    LD_KERNEL_START = .;

    /* Code. */
    .text BLOCK(4K) : ALIGN(4K) {
        LD_KERNEL_TEXT_START = .;
        *(.multiboot_header)
        *(.boot_kernel)
        *(.text)
        . = ALIGN(4096);
        LD_KERNEL_TEXT_END = .;
    }

    /* Read-only data. */
    .rodata BLOCK(4K) : ALIGN(4K) {
        LD_KERNEL_RODATA_START = .;
        *(.rodata)
        . = ALIGN(4096);
        LD_KERNEL_RODATA_END = .;
    }

    /* Read-write data (initialized) */
    .data BLOCK(4K) : ALIGN(4K) {
        LD_KERNEL_DATA_START = .;
        *(.data)
        . = ALIGN(4096);
        LD_KERNEL_DATA_END = .;
    }

    /* Read-write data (uninitialized) and stack */
    .bss BLOCK(4K) : ALIGN(4K) {
        /* LD_KERNEL_BSS_START = .; */
        *(.kernel_init_stack)
        *(COMMON)
        *(.bss)
        . = ALIGN(4096);
        /* LD_KERNEL_BSS_END = .; */
    }

    LD_KERNEL_END = .;

    LD_KERNEL_SIZE = LD_KERNEL_END - LD_KERNEL_START;
}
