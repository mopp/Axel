/**
 * @file dev/ata.c
 * @brief ata implementation.
 * @author mopp
 * @version 0.1
 * @date 2014-10-20
 */

#include <dev/pci.h>
#include <dev/ata.h>
#include <utils.h>
#include <asm_functions.h>
#include <macros.h>


struct ata {
    uint16_t command_base; /* Base port number (which is used for read/write "ATA command block register"). */
    uint16_t control_base; /* Base port number (which is used for read/write "ATA control block register"). */
    uint16_t bus_master;   /* Bus Master IDE */
    uint8_t n_ien;         /* nIEN (No Interrupt); */
    struct {
        uint8_t reserved;      /* 0 (Empty) or 1 (This Drive really exists). */
        uint8_t type;          /* 0: ATA, 1:ATAPI. */
        uint16_t signature;    /* Drive Signature */
        uint16_t capabilities; /* Features. */
        uint32_t command_sets; /* Command Sets Supported. */
        uint32_t size;         /* Size in Sectors. */
    } dev[2]; /* Master and slave */
};
typedef struct ata Ata;


enum ata_enum {
    ATA_PRIMARY          = 0x00,
    ATA_SECONDARY        = 0x01,
    ATA_MASTER           = 0x00,
    ATA_SLAVE            = 0x01,

    ATA_ATA              = 0x00,
    ATA_ATAPI            = 0x01,

    /* These are offset of command block register. */
    ATA_REG_DATA         = 0x00,
    ATA_REG_ERROR        = 0x01,
    ATA_REG_FEATURES     = 0x01,
    ATA_REG_SECTOR_COUNT = 0x02,
    ATA_REG_INTR_RESAON  = 0x02,
    ATA_REG_LBA_LOW      = 0x03,
    ATA_REG_LBA_MID      = 0x04,
    ATA_REG_LBA_HIGH     = 0x05,
    ATA_REG_BCNT_LOW     = 0x04,
    ATA_REG_BCNT_HIGH    = 0x05,
    ATA_REG_DEV          = 0x06,
    ATA_REG_DEV_SELECT   = 0x06,
    ATA_REG_STATUS       = 0x07,
    ATA_REG_COMMAND      = 0x07,

    /*
     * These are offset of control block register.
     * Shift and adding 0xaa is trick for identify which register.
     */
    ATA_REG_CTRL_MARK    = 0xaa,
    ATA_REG_ALT_STATUS   = (0x00 << 4) + ATA_REG_CTRL_MARK,
    ATA_REG_DEV_CONTROL  = (0x00 << 4) + ATA_REG_CTRL_MARK,

    /* These are command code. */
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

    ATA_READ             = 0x00,
    ATA_WRITE            = 0x01,
};
typedef enum ata_enum Ata_enum;


static Ata atas[2]; /* primary and secondary. */
static uint8_t buffer[2048];
static uint8_t ide_irq_invoked;
static uint8_t atapi_packet[12] = {0xA8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static inline uint16_t get_port_number(Ata_enum channel, Ata_enum reg) {
    uint16_t port;

    if ((reg & 0xff) == ATA_REG_CTRL_MARK) {
        /* Register that is written is control block register.  */
        port = atas[channel].control_base + (reg >> 4);
    } else {
        port = atas[channel].command_base + reg;
    }

    return port;
}


static inline void ata_write(Ata_enum channel, Ata_enum reg, uint8_t data) {
    io_out8(get_port_number(channel, reg), data);
}


static inline uint8_t ata_read(Ata_enum channel, Ata_enum reg) {
    return io_in8(get_port_number(channel, reg));
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


Axel_state_code init_ata(void) {
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

    uint16_t base_addrs[5] = {
        ECAST_UINT16(pci_read_addr_data(&pcr, 0x10).h0.reg_10.base_addr0),
        ECAST_UINT16(pci_read_addr_data(&pcr, 0x14).h0.reg_14.base_addr1),
        ECAST_UINT16(pci_read_addr_data(&pcr, 0x18).h0.reg_18.base_addr2),
        ECAST_UINT16(pci_read_addr_data(&pcr, 0x1c).h0.reg_1c.base_addr3),
        ECAST_UINT16(pci_read_addr_data(&pcr, 0x20).h0.reg_20.base_addr4),
    };

    for (size_t i = 0; i < ARRAY_SIZE_OF(base_addrs) - 1; i++) {
        if (!(base_addrs[i] == 0 || base_addrs[i] == 1)) {
            return AXEL_FAILED;
        }
    }

    /* Detect I/O Ports which interface IDE Controller: */
    atas[ATA_PRIMARY].command_base   = (base_addrs[0] & 0xFFFC) + (0x1F0 * ((base_addrs[0] == 0) ? 1 : 0));
    atas[ATA_PRIMARY].control_base   = (base_addrs[1] & 0xFFFC) + (0x3F6 * ((base_addrs[1] == 0) ? 1 : 0));
    atas[ATA_PRIMARY].bus_master     = (base_addrs[4] & 0xFFFC) + 0;
    atas[ATA_SECONDARY].command_base = (base_addrs[2] & 0xFFFC) + (0x170 * ((base_addrs[2] == 0) ? 1 : 0));
    atas[ATA_SECONDARY].control_base = (base_addrs[3] & 0xFFFC) + (0x376 * ((base_addrs[3] == 0) ? 1 : 0));
    atas[ATA_SECONDARY].bus_master   = (base_addrs[4] & 0xFFFC) + 8;

    puts("ATA Primary\n");
    printf("  0x%x\n", atas[ATA_PRIMARY].command_base);
    printf("  0x%x\n", atas[ATA_PRIMARY].control_base);
    printf("  0x%x\n", atas[ATA_PRIMARY].bus_master);
    puts("ATA Secondary\n");
    printf("  0x%x\n", atas[ATA_SECONDARY].command_base);
    printf("  0x%x\n", atas[ATA_SECONDARY].control_base);
    printf("  0x%x\n", atas[ATA_SECONDARY].bus_master);

    /* Disable IRQs */
    ata_write(ATA_PRIMARY, ATA_REG_DEV_CONTROL, 2);
    ata_write(ATA_SECONDARY, ATA_REG_DEV_CONTROL, 2);

#if 0
    // 3- Detect ATA-ATAPI Devices:
    int i, j, count = 0;
    for (i = 0; i < 2; i++)
        for (j = 0; j < 2; j++) {
            unsigned char err = 0, type = IDE_ATA, status;
            devices[count].reserved = 0;  // Assuming that no drive here.

            // (I) Select Drive:
            ide_write(i, ATA_REG_HDDEVSEL, 0xA0 | (j << 4));  // Select Drive.
            /* sleep(1); // Wait 1ms for drive select to work. */

            // (II) Send ATA Identify Command:
            ide_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
            /* sleep(1); // This function should be implemented in your OS. which waits for 1 ms. */
            // it is based on System Timer Device Driver.

            // (III) Polling:
            if (ide_read(i, ATA_REG_STATUS) == 0)
                continue;  // If Status = 0, No Device.

            while (1) {
                status = ide_read(i, ATA_REG_STATUS);
                if ((status & ATA_SR_ERR)) {
                    err = 1;
                    break;
                }  // If Err, Device is not ATA.
                if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ))
                    break;  // Everything is right.
            }

            // (IV) Probe for ATAPI Devices:

            if (err != 0) {
                unsigned char cl = ide_read(i, ATA_REG_LBA1);
                unsigned char ch = ide_read(i, ATA_REG_LBA2);

                if (cl == 0x14 && ch == 0xEB)
                    type = IDE_ATAPI;
                else if (cl == 0x69 && ch == 0x96)
                    type = IDE_ATAPI;
                else
                    continue;  // Unknown Type (may not be a device).

                ide_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
                /* sleep(1); */
            }

            // (V) Read Identification Space of the Device:
            /* ide_read_buffer(i, ATA_REG_DATA, (unsigned int) buffer, 128); */

            // (VI) Read Device Parameters:
            devices[count].reserved = 1;
            devices[count].type = type;
            devices[count].channel = i;
            devices[count].drive = j;
            devices[count].signature = *((unsigned short *)(buffer + ATA_IDENT_DEVICETYPE));
            devices[count].capabilities = *((unsigned short *)(buffer + ATA_IDENT_CAPABILITIES));
            devices[count].command_sets = *((unsigned int *)(buffer + ATA_IDENT_COMMANDSETS));

            // (VII) Get Size:
            if (devices[count].command_sets & (1 << 26))
                // Device uses 48-Bit Addressing:
                devices[count].size = *((unsigned int *)(buffer + ATA_IDENT_MAX_LBA_EXT));
            else
                // Device uses CHS or 28-bit Addressing:
                devices[count].size = *((unsigned int *)(buffer + ATA_IDENT_MAX_LBA));

            // (VIII) String indicates model of device (like Western Digital HDD and SONY DVD-RW...):
            for (int k = 0; k < 40; k += 2) {
                devices[count].model[k] = buffer[ATA_IDENT_MODEL + k + 1];
                devices[count].model[k + 1] = buffer[ATA_IDENT_MODEL + k];
            }
            devices[count].model[40] = 0;  // Terminate String.

            count++;
        }

    // 4- Print Summary:
    for (i = 0; i < 4; i++)
        if (devices[i].reserved == 1) {
            printf(" Found %s Drive %dGB - %s\n",
                    (const char *[]) { "ATA", "ATAPI" }[devices[i].type], /* Type */
                    devices[i].size / 1024 / 1024 / 2,                    /* Size */
                    devices[i].model);
        }
#endif

    return AXEL_SUCCESS;
}
