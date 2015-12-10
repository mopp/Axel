/*
 * vim:ft=gas:
 * @brief Booting kernel main routine.
 * @author mopp
 * @version 0.2
 * @date 2015-12-10
 */

/*
 * Entry point for the kernel.
 * r15 -> should begin execution at 0x8000.
 * r0  -> 0x00000000
 * r1  -> 0x00000C42
 * r2  -> 0x00000100 - start of ATAGS
 * preserve these registers as argument for kernel_main
 */
.section .boot_kernel
.globl boot_kernel
boot_kernel:
    /* Setup the stack. */
    mov sp, #0x8000

    /* Clear out bss. */
    ldr r4, =LD_KERNEL_BSS_BEGIN
    ldr r9, =LD_KERNEL_BSS_END
    mov r5, #0
    mov r6, #0
    mov r7, #0
    mov r8, #0
    b       2f

1:
    /* Store multiple at r4. */
    stmia r4!, {r5-r8}

2:
    cmp r4, r9
    blo 1b

    ldr r3, =main
    blx r3

halt:
    wfe
    b halt
