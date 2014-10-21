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


struct ide_channel {
    uint16_t base_io_port;   /* I/O Base. */
    uint16_t base_ctrl_port; /* Control Base */
    uint16_t bus_master;     /* Bus Master IDE */
    uint8_t n_ien;           /* nIEN (No Interrupt); */
    uint8_t status;
};
typedef struct ide_channel Ide_channel;


struct ide_device {
    uint8_t reserved;      /* 0 (Empty) or 1 (This Drive really exists). */
    uint8_t channel;       /* 0 (Primary Channel) or 1 (Secondary Channel). */
    uint8_t drive;         /* 0 (Master Drive) or 1 (Slave Drive). */
    uint16_t type;         /* 0: ATA, 1:ATAPI. */
    uint16_t signature;    /* Drive Signature */
    uint16_t capabilities; /* Features. */
    uint32_t command_sets; /* Command Sets Supported. */
    uint32_t size;         /* Size in Sectors. */
    uint8_t model[41];     /* Model in string. */
};
typedef struct ide_channel Ide_device;


enum {
    ATA_SR_BSY              = 0x80,
    ATA_SR_DRDY             = 0x40,
    ATA_SR_DF               = 0x20,
    ATA_SR_DSC              = 0x10,
    ATA_SR_DRQ              = 0x08,
    ATA_SR_CORR             = 0x04,
    ATA_SR_IDX              = 0x02,
    ATA_SR_ERR              = 0x01,

    ATA_ER_BBK              = 0x80,
    ATA_ER_UNC              = 0x40,
    ATA_ER_MC               = 0x20,
    ATA_ER_IDNF             = 0x10,
    ATA_ER_MCR              = 0x08,
    ATA_ER_ABRT             = 0x04,
    ATA_ER_TK0NF            = 0x02,
    ATA_ER_AMNF             = 0x01,

    ATA_CMD_READ_PIO        = 0x20,
    ATA_CMD_READ_PIO_EXT    = 0x24,
    ATA_CMD_READ_DMA        = 0xC8,
    ATA_CMD_READ_DMA_EXT    = 0x25,
    ATA_CMD_WRITE_PIO       = 0x30,
    ATA_CMD_WRITE_PIO_EXT   = 0x34,
    ATA_CMD_WRITE_DMA       = 0xCA,
    ATA_CMD_WRITE_DMA_EXT   = 0x35,
    ATA_CMD_CACHE_FLUSH     = 0xE7,
    ATA_CMD_CACHE_FLUSH_EXT = 0xEA,
    ATA_CMD_PACKET          = 0xA0,
    ATA_CMD_IDENTIFY_PACKET = 0xA1,
    ATA_CMD_IDENTIFY        = 0xEC,

    ATAPI_CMD_READ          = 0xA8,
    ATAPI_CMD_EJECT         = 0x1B,

    ATA_IDENT_DEVICETYPE    = 0,
    ATA_IDENT_CYLINDERS     = 2,
    ATA_IDENT_HEADS         = 6,
    ATA_IDENT_SECTORS       = 12,
    ATA_IDENT_SERIAL        = 20,
    ATA_IDENT_MODEL         = 54,
    ATA_IDENT_CAPABILITIES  = 98,
    ATA_IDENT_FIELDVALID    = 106,
    ATA_IDENT_MAX_LBA       = 120,
    ATA_IDENT_COMMANDSETS   = 164,
    ATA_IDENT_MAX_LBA_EXT   = 200,

    IDE_ATA                 = 0x00,
    IDE_ATAPI               = 0x01,

    ATA_MASTER              = 0x00,
    ATA_SLAVE               = 0x01,

    ATA_REG_DATA            = 0x00,
    ATA_REG_ERROR           = 0x01,
    ATA_REG_FEATURES        = 0x01,
    ATA_REG_SECCOUNT0       = 0x02,
    ATA_REG_LBA0            = 0x03,
    ATA_REG_LBA1            = 0x04,
    ATA_REG_LBA2            = 0x05,
    ATA_REG_HDDEVSEL        = 0x06,
    ATA_REG_COMMAND         = 0x07,
    ATA_REG_STATUS          = 0x07,
    ATA_REG_SECCOUNT1       = 0x08,
    ATA_REG_LBA3            = 0x09,
    ATA_REG_LBA4            = 0x0A,
    ATA_REG_LBA5            = 0x0B,
    ATA_REG_CONTROL         = 0x0C,
    ATA_REG_ALTSTATUS       = 0x0C,
    ATA_REG_DEVADDRESS      = 0x0D,

    // Channels:
    ATA_PRIMARY             = 0x00,
    ATA_SECONDARY           = 0x01,

    // Directions:
    ATA_READ                = 0x00,
    ATA_WRITE               = 0x01,
};


static Ide_channel channels[2]; /* primary and secondary. */
static uint8_t buffer[2048] = {0};
static uint8_t ide_irq_invoked = 0;
static uint8_t atapi_packet[12] = {0xA8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};


void ide_write(unsigned char channel, unsigned char reg, unsigned char data) {
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].n_ien);
    if (reg < 0x08)
        io_out8(channels[channel].base_io_port  + reg - 0x00, data);
    else if (reg < 0x0C)
        io_out8(channels[channel].base_io_port  + reg - 0x06, data);
    else if (reg < 0x0E)
        io_out8(channels[channel].base_ctrl_port  + reg - 0x0A, data);
    else if (reg < 0x16)
        io_out8(channels[channel].bus_master + reg - 0x0E, data);
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, channels[channel].n_ien);
}


unsigned char ide_read(unsigned char channel, unsigned char reg) {
    unsigned char result = 0;
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].n_ien);
    if (reg < 0x08)
        result = io_in8(channels[channel].base_io_port + reg - 0x00);
    else if (reg < 0x0C)
        result = io_in8(channels[channel].base_io_port  + reg - 0x06);
    else if (reg < 0x0E)
        result = io_in8(channels[channel].base_ctrl_port  + reg - 0x0A);
    else if (reg < 0x16)
        result = io_in8(channels[channel].bus_master + reg - 0x0E);
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, channels[channel].n_ien);

    return result;
}


#if 0
void ide_read_buffer(unsigned char channel, unsigned char reg, unsigned int buffer, unsigned int quads) {
    /* WARNING: This code contains a serious bug. The inline assembly trashes ES and
     *           ESP for all of the code the compiler generates between the inline
     *           assembly blocks.
     */
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].n_ien);

    __asm__("pushw %es; movw %ds, %ax; movw %ax, %es");
    if (reg < 0x08)
        insl(channels[channel].base_io_port  + reg - 0x00, buffer, quads);
    else if (reg < 0x0C)
        insl(channels[channel].base_io_port  + reg - 0x06, buffer, quads);
    else if (reg < 0x0E)
        insl(channels[channel].base_ctrl_port  + reg - 0x0A, buffer, quads);
    else if (reg < 0x16)
        insl(channels[channel].bus_master + reg - 0x0E, buffer, quads);
    __asm__("popw %es;");

    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, channels[channel].n_ien);
}
#endif


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
        printf("IRQ is 14 and 15\n");
    } else {
        /* This device needs an IRQ assignment. */
        return AXEL_FAILED;
    }

    pdr = pci_read_addr_data(&pcr, 0x10);
    pdr = pci_read_addr_data(&pcr, 0x14);
    pdr = pci_read_addr_data(&pcr, 0x18);
    pdr = pci_read_addr_data(&pcr, 0x1c);
    pdr = pci_read_addr_data(&pcr, 0x20);
    printf("Base 0: 0x%x\n", pdr.h0.reg_10.base_addr0);
    printf("Base 1: 0x%x\n", pdr.h0.reg_14.base_addr1);
    printf("Base 2: 0x%x\n", pdr.h0.reg_18.base_addr2);
    printf("Base 3: 0x%x\n", pdr.h0.reg_1c.base_addr3);
    printf("Base 4: 0x%x\n", pdr.h0.reg_20.base_addr4);

    return AXEL_SUCCESS;
}
