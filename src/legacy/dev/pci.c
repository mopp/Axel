/**
 * @file pci.c
 * @brief PCI implementation.
 * @author mopp
 * @version 0.1
 * @date 2014-10-20
 */

#include <dev/pci.h>
#include <asm_functions.h>
#include <stdbool.h>
#include <macros.h>


Pci_data_reg pci_read_data(Pci_conf_reg const pcr) {
    io_out32(PCI_CONFIG_ADDR_PORT, pcr.bit_expr);
    io_hlt();
    Pci_data_reg d = {.bit_expr = io_in32(PCI_CONFIG_DATA_PORT)};
    return d;
}


Pci_data_reg pci_read_addr_data(Pci_conf_reg* const pcr, uint8_t addr) {
    pcr->reg_addr = addr;
    return pci_read_data(*pcr);
}


void pci_write_data(uint8_t d) {
    io_hlt();
    io_out8(PCI_CONFIG_DATA_PORT, d);
}


Pci_conf_reg find_pci_dev(Pci_constants class, Pci_constants sub_class) {
    Pci_conf_reg pcr = {.bit_expr = io_in32(PCI_CONFIG_ADDR_PORT)};

    for (uint16_t i = 0; i <= PCI_MAX_BUS_NUMBER; i++) {
        pcr.bus_num = ECAST_UINT8(i);
        for (uint16_t j = 0; j <= PCI_MAX_DEVICE_NUMBER; j++) {
            pcr.dev_num    = ECAST_UINT8(j);
            pcr.reg_addr   = 0x00;
            pcr.func_num   = 0x00;
            Pci_data_reg d = pci_read_data(pcr);

            if (PCI_INVALID_DATA != d.bit_expr) {
                pcr.reg_addr = 0x0c;
                d = pci_read_data(pcr);

                pcr.reg_addr = 0x08;
                bool is_multi = ((PCI_INVALID_DATA != d.bit_expr) && (d.reg_0c.enable_multi_func == 1)) ? true : false;
                uint8_t k = 0;
                do {
                    pcr.func_num = k;
                    d = pci_read_data(pcr);

                    if (PCI_INVALID_DATA != d.bit_expr && d.reg_08.class == class && d.reg_08.sub_class == sub_class) {
                        return pcr;
                    }
                    ++k;
                } while ((is_multi == true) && (k <= PCI_MAX_FUNCTION_NUMBER));
            }
        }
    }

    pcr.bit_expr = 0;
    return pcr;
}
