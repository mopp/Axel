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


Axel_state_code init_ide(void) {
    // Pci_conf_reg pcr = find_ide_controler();
    // printf("bus num %x\n", pcr.bus_num);
    // printf("func num %x\n", pcr.func_num);
    // pcr.reg_addr = 0x3c;
    // Pci_data_reg pdr = pci_read_data(pcr);
    // io_hlt();
    // io_out8(PCI_CONFIG_DATA_PORT, 0xFE);
    // pdr = pci_read_data(pcr);
    // printf("interrupt line : 0x%x\n", pdr.h0.reg_3c.interrupt_line);
    // printf("interrupt pin  : 0x%x\n", pdr.h0.reg_3c.interrupt_pin);

    putchar('\n');


    return AXEL_SUCCESS;
}
