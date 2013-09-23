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
    kernelStart  =  .;

    /* Read-only data. */
    .rodata BLOCK(4K) : ALIGN(4K) {
        *(.multiboot_header)
        *(.rodata)
    }

    /* Code. */
    .text BLOCK(4K) : ALIGN(4K) {
        *(.boot_kernel)
        *(.text)
    }

    /* Read-write data (initialized) */
    .data BLOCK(4K) : ALIGN(4K) {
        *(.data)
    }

    /* Read-write data (uninitialized) and stack */
    .bss BLOCK(4K) : ALIGN(4K) {
        *(.kernel_init_stack)
        *(COMMON)
        *(.bss)
    }
}
