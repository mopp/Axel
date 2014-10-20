/**
 * @file dev/ide.c
 * @brief ide implementation.
 * @author mopp
 * @version 0.1
 * @date 2014-10-20
 */

#include <dev/pci.h>
#include <dev/ide.h>
#include <utils.h>
#include <asm_functions.h>


struct IDEChannelRegisters {
   unsigned short base;  // I/O Base.
   unsigned short ctrl;  // Control Base
   unsigned short bmide; // Bus Master IDE
   unsigned char  nIEN;  // nIEN (No Interrupt);
} channels[2];


Axel_state_code init_ide(void) {
    Pci_conf_reg pcr = find_pci_dev(PCI_CLASS_MASS_STORAGE, PCI_SUBCLASS_IDE);

    putchar('\n');
    printf("bus num %x\n", pcr.bus_num);
    printf("func num %x\n", pcr.func_num);

    /* Read IRQ line. */
    Pci_data_reg pdr = pci_read_addr_data(&pcr, 0x3c);
    pci_write_data(0xfe);
    pdr = pci_read_data(pcr);

    if ((pdr.h0.reg_3c.interrupt_line == 0) && (pdr.h0.reg_3c.interrupt_pin == 0)) {
        /* This is a Parallel IDE Controller which uses IRQs 14 and 15. */
    } else {
        /* This device needs an IRQ assignment. */
        return AXEL_FAILED;
    }

    pdr = pci_read_addr_data(&pcr, 0x10);
    printf("Base 0: 0x%x\n", pdr.h0.reg_10.base_addr0);

    return AXEL_SUCCESS;
}
