/**
 * @file exception.c
 * @brief exception handlers.
 * @author mopp
 * @version 0.1
 * @date 2014-08-17
 */


#include <exception.h>
#include <asm_functions.h>
#include <paging.h>
#include <macros.h>
#include <interrupt.h>
#include <utils.h>



void exception_page_fault(Interrupt_frame* ic) {
    uintptr_t fault_addr = load_cr2();
    io_cli();
    BOCHS_MAGIC_BREAK();

    if (fault_addr < KERNEL_VIRTUAL_BASE_ADDR) {
        /* TODO: User space fault */
        io_cli();
        printf("\nfault_addr: 0x%zx\n", fault_addr);
        puts("User space fault or Kernel space fault");
        DIRECTLY_WRITE_STOP(uintptr_t, KERNEL_VIRTUAL_BASE_ADDR, 0x1);
    }

    /* Kernel space fault */
    if (is_kernel_pdt((Page_directory_table)(phys_to_vir_addr(get_cpu_pdt()))) == true) {
        /* Maybe kernel error */
        /* TODO: panic */
        io_cli();
        printf("\nfault_addr: 0x%zx\n", fault_addr);
        puts("Kernel space fault");
        DIRECTLY_WRITE_STOP(uintptr_t, KERNEL_VIRTUAL_BASE_ADDR, 0x2);
    }

    Axel_state_code result = synchronize_pdt(fault_addr);
    if (result != AXEL_SUCCESS) {
        /* TODO: panic */
        io_cli();
        printf("\nfault_addr: 0x%zx\n", fault_addr);
        puts("Synchronize fault");
        DIRECTLY_WRITE_STOP(uintptr_t, KERNEL_VIRTUAL_BASE_ADDR, 0x3);
    }
}
