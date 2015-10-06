/**
 * @file pci.h
 * @brief PCI header.
 * @author mopp
 * @version 0.1
 * @date 2014-10-20
 */

#ifndef _PCI_H_
#define _PCI_H_



#include <stdint.h>

enum pci_constants {
    PCI_CONFIG_ADDR_PORT    = 0xcf8,
    PCI_CONFIG_DATA_PORT    = 0xcfc,
    PCI_MAX_BUS_NUMBER      = 255,
    PCI_MAX_DEVICE_NUMBER   = 31,
    PCI_MAX_FUNCTION_NUMBER = 7,
    PCI_INVALID_DATA        = 0xffffffff,

    PCI_CLASS_MASS_STORAGE  = 0x01,
    PCI_SUBCLASS_IDE        = 0x01,
};
typedef enum pci_constants Pci_constants;


union pci_conf_reg {
    struct {
        uint8_t reg_addr;
        uint8_t func_num : 3;
        uint8_t dev_num : 5;
        uint8_t bus_num;
        uint8_t reserved : 7;
        uint8_t enable : 1;
    };
    uint32_t bit_expr;
};
typedef union pci_conf_reg Pci_conf_reg;
_Static_assert(sizeof(Pci_conf_reg) == 4, "");


union pci_data_reg {
    struct {
        uint16_t vender_id;
        uint16_t device_id;
    } reg_00;
    struct {
        uint16_t command;
        uint16_t status;
    } reg_04;
    struct {
        uint8_t revision_id;
        uint8_t prog_if;
        uint8_t sub_class;
        uint8_t class;
    } reg_08;
    struct {
        uint8_t cache_line_size;
        uint8_t latency_timer;
        uint8_t header_type : 7;
        uint8_t enable_multi_func:1;
        uint8_t bist;
    } reg_0c;
    union {
        struct {
            uint32_t base_addr0;
        } reg_10;
        struct {
            uint32_t base_addr1;
        } reg_14;
        struct {
            uint32_t base_addr2;
        } reg_18;
        struct {
            uint32_t base_addr3;
        } reg_1c;
        struct {
            uint32_t base_addr4;
        } reg_20;
        struct {
            uint32_t base_addr5;
        } reg_24;
        struct {
            uint8_t max_latency;
            uint8_t min_grant;
            uint8_t interrupt_pin;
            uint8_t interrupt_line;
        } reg_3c;
    } h0;
    union {
    } h1;
    union {
    } h2;
    uint32_t bit_expr;
};
typedef union pci_data_reg Pci_data_reg;
_Static_assert(sizeof(Pci_data_reg) == 4, "");


extern Pci_data_reg pci_read_data(Pci_conf_reg const);
extern Pci_data_reg pci_read_addr_data(Pci_conf_reg* const, uint8_t);
extern void pci_write_data(uint8_t);
extern Pci_conf_reg find_pci_dev(Pci_constants, Pci_constants);



#endif
