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



void exception_page_fault(Interrupt_context* ic) {
    uintptr_t fault_addr = load_cr2();

    if (fault_addr < KERNEL_VIRTUAL_BASE_ADDR) {
        /* TODO: User space fault */
        io_cli();
        DIRECTLY_WRITE_STOP(uintptr_t, KERNEL_VIRTUAL_BASE_ADDR, 0x1);
    }

    /* Kernel space fault */
    Page_directory_table pdt = get_cpu_pdt();
    if (is_kernel_pdt(pdt) == true) {
        /* Maybe kernel error */
        /* TODO: panic */
        io_cli();
        DIRECTLY_WRITE_STOP(uintptr_t, KERNEL_VIRTUAL_BASE_ADDR, 0x2);
    }

    Axel_state_code result = synchronize_pdt(pdt, fault_addr);
    if (result != AXEL_SUCCESS) {
        /* TODO: panic */
        io_cli();
        DIRECTLY_WRITE_STOP(uintptr_t, KERNEL_VIRTUAL_BASE_ADDR, 0x3);
    }
}
