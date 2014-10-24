/**
 * @file dev/ata.c
 * @brief ata implementation.
 *        reference is ATA-7 specification.
 *        http://www.t13.org/Documents/UploadedDocuments/docs2007/D1532v1r4b-AT_Attachment_with_Packet_Interface_-_7_Volume_1.pdf
 *        http://www.t13.org/Documents/UploadedDocuments/docs2007/D1532v2r4b-AT_Attachment_with_Packet_Interface_-_7_Volume_2.pdf
 *        http://www.t13.org/Documents/UploadedDocuments/docs2007/D1532v3r4b-AT_Attachment_with_Packet_Interface_-_7_Volume_3.pdf
 *        TODO: add ATAPI function.
 * @author mopp
 * @version 0.1
 * @date 2014-10-20
 */

#include <dev/pci.h>
#include <dev/ata.h>
#include <utils.h>
#include <asm_functions.h>
#include <macros.h>
#include <interrupt.h>
#include <stdbool.h>
#include <paging.h>

struct dev {
    uint8_t is_exists; /* 0 (Empty) or 1 (This Drive really exists). */
    uint8_t type;      /* ATA, ATAPI or Unknown. */
    uint16_t gen_conf; /* General configuration bit-significant information: */
    uint16_t capabilities; /* Features. */
    uint32_t command_sets; /* Command Sets Supported. */
    uint32_t sector_nr;    /* How many sectors. */
    uint32_t sector_size;  /* Byte size per 1 sector. */
};
typedef struct dev Dev;


struct ata {
    uint16_t command_base; /* Base port number (which is used for read/write "ATA command block register"). */
    uint16_t control_base; /* Base port number (which is used for read/write "ATA control block register"). */
    uint16_t bus_master;   /* Bus Master IDE */
    uint8_t n_ien;         /* nIEN (No Interrupt); */
    Dev dev[2];            /* Master and slave */
};
typedef struct ata Ata;


union status_register {
    struct {
        uint8_t err_chk : 1;   /* Error / Check . */
        uint8_t obsoleted : 2; /* Obsoleted . */
        uint8_t drq : 1;       /* Data requested. */
        uint8_t cmd_dep : 1;   /* Command depended. */
        uint8_t df_se : 1;     /* Device Fault / Stream Error */
        uint8_t drdy : 1;      /* Device ready */
        uint8_t bsy : 1;       /* Busy */
    };
    uint8_t bit_expr;
};
typedef union status_register Status_register;
_Static_assert(sizeof(Status_register) == 1, "Status_register is NOT 1 byte");


enum {
    ATA_PRIMARY                 = 0x00,
    ATA_SECONDARY               = 0x01,
    ATA_MASTER                  = 0x00,
    ATA_SLAVE                   = 0x01,

    TYPE_ATA                    = 0x00,
    TYPE_ATAPI                  = 0x01,
    TYPE_UNKNOWN                = 0x02,
    ATA_ERROR                   = 0x20,

    /* These are offset of command block register. */
    ATA_REG_DATA                = 0x00,
    ATA_REG_ERROR               = 0x01,
    ATA_REG_FEATURES            = 0x01,
    ATA_REG_SECTOR_COUNT        = 0x02,
    ATA_REG_INTR_RESAON         = 0x02,
    ATA_REG_LBA0                = 0x03,
    ATA_REG_LBA1                = 0x04,
    ATA_REG_LBA2                = 0x05,
    ATA_REG_BCNT_LOW            = 0x04,
    ATA_REG_BCNT_HIGH           = 0x05,
    ATA_REG_DEV                 = 0x06,
    ATA_REG_DEV_SELECT          = 0x06,
    ATA_REG_STATUS              = 0x07,
    ATA_REG_COMMAND             = 0x07,

    /*
     * These are offset of control block register.
     * Shift and adding 0xaa is trick for identify which register.
     */
    ATA_REG_CTRL_MARK           = 0xaa,
    ATA_REG_ALT_STATUS          = (0x00 << 8) | ATA_REG_CTRL_MARK,
    ATA_REG_DEV_CTRL            = (0x00 << 8) | ATA_REG_CTRL_MARK,

    ATA_REG_STATUS_ERR_CHK      = 0x01,
    ATA_REG_STATUS_DRQ          = 0x08,
    ATA_REG_STATUS_CMD_DEP      = 0x10,
    ATA_REG_STATUS_DF_SE        = 0x20,
    ATA_REG_STATUS_DRDY         = 0x40,
    ATA_REG_STATUS_BSY          = 0x80,

    ATA_FIELD_DEV_CTRL_N_IEN    = 0x02,
    ATA_FIELD_DEV_CTRL_SRST     = 0x04,
    ATA_FIELD_DEV_CTRL_HOB      = 0x80,

    /* These are command code. */
    ATA_CMD_READ_PIO            = 0x20,
    ATA_CMD_READ_PIO_EXT        = 0x24,
    ATA_CMD_READ_DMA            = 0xC8,
    ATA_CMD_READ_DMA_EXT        = 0x25,
    ATA_CMD_WRITE_PIO           = 0x30,
    ATA_CMD_WRITE_PIO_EXT       = 0x34,
    ATA_CMD_WRITE_DMA           = 0xCA,
    ATA_CMD_WRITE_DMA_EXT       = 0x35,
    ATA_CMD_CACHE_FLUSH         = 0xE7,
    ATA_CMD_CACHE_FLUSH_EXT     = 0xEA,
    ATA_CMD_PACKET              = 0xA0,
    ATA_CMD_IDENTIFY_PACKET     = 0xA1,
    ATA_CMD_IDENTIFY            = 0xEC,

    ATA_READ                    = 0x00,
    ATA_WRITE                   = 0x01,

    ATA_IDENTIFY_GEN_CONF       = 0,
    ATA_IDENTIFY_CAPABILITIES   = 49,
    ATA_IDENTIFY_SECTOR_NR0     = 60,
    ATA_IDENTIFY_SECTOR_NR1     = 61,
    ATA_IDENTIFY_MAJOR_VERSION  = 80,
    ATA_IDENTIFY_CMD_SET0       = 82,
    ATA_IDENTIFY_CMD_SET1       = 83,
    ATA_IDENTIFY_CMD_SET_EXT    = 84,
    ATA_IDENTIFY_LBA48_MAX0     = 100,
    ATA_IDENTIFY_LBA48_MAX1     = 101,
    ATA_IDENTIFY_LBA48_MAX2     = 102,
    ATA_IDENTIFY_LBA48_MAX3     = 103,
    ATA_IDENTIFY_SECTOR_SIZE    = 106,
    ATA_IDENTIFY_LSECTOR_SIZE0  = 117,
    ATA_IDENTIFY_LSECTOR_SIZE1  = 118,
    ATA_IDENTIFY_DATA_WORD_SIZE = 256,

    ATA_CMD_SET_LBA48           = 0x04000000,
    ATA_CAPABILITIES_LBA        = 0x0020,
};
typedef enum ata_enum Ata_enum;


static Ata atas[2]; /* primary and secondary. */
static uint16_t id_buffer[ATA_IDENTIFY_DATA_WORD_SIZE];
static uint8_t ide_irq_invoked;
static uint8_t atapi_packet[12] = {0xA8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};


static inline uint16_t get_port_number(uint8_t channel, uint8_t reg) {
    uint16_t port;

    if ((reg & 0xff) == ATA_REG_CTRL_MARK) {
        /* Register that is written is control block register.  */
        port = atas[channel].control_base + ECAST_UINT16(reg >> 8);
    } else {
        port = atas[channel].command_base + ECAST_UINT16(reg);
    }

    return port;
}


static inline void ata_write(uint8_t channel, uint8_t reg, uint8_t data) {
    io_out8(get_port_number(channel, reg), data);
}


static inline uint8_t ata_read(uint8_t channel, uint8_t reg) {
    return io_in8(get_port_number(channel, reg));
}


#if 0
static inline void ata_write_data_reg(uint8_t channel, uint16_t data) {
    io_out16(get_port_number(channel, ATA_REG_DATA), data);
}


static inline uint16_t ata_read_data_reg(uint8_t channel) {
    return io_in16(get_port_number(channel, ATA_REG_DATA));
}
#endif


static inline void ata_wait_status(uint8_t channel, uint8_t mask, uint8_t value) {
    Status_register sr;
    do {
        sr.bit_expr = ata_read(channel, ATA_REG_ALT_STATUS);
    } while ((sr.bit_expr & mask) != value);
}


static inline void ata_wait400ns(uint8_t ch) {
    /*
     * Wait 400 nanosecond for BSY to be set
     * Reading the Alternate Status port wastes 100ns; loop four times.
     */
    for (int i = 0; i < 4; i++) {
        ata_read(ch, ATA_REG_ALT_STATUS);
    }
}


static inline uint8_t ata_polling(uint8_t ch, bool is_check) {
    Status_register sr;

    ata_wait400ns(ch);
    ata_wait_status(ch, ATA_REG_STATUS_BSY, 0);

    do {
        sr.bit_expr = ata_read(ch, ATA_REG_STATUS);
        if (sr.df_se == 1) {
            return ATA_REG_STATUS_DF_SE;
        } else if (sr.err_chk == 1) {
            return ATA_REG_STATUS_ERR_CHK;
        }
    } while (sr.drq == 0 && is_check == true);

    /* empty read */
    ata_read(ch, ATA_REG_ALT_STATUS);

    return 0;
}


static inline void ata_dev_reset(uint8_t ch) {
    /*
     * Reset channel and disable IRQ
     * We shall wait over 5us after setting SRST bit to 1.
     */
    ata_write(ch, ATA_REG_DEV_CTRL, ATA_FIELD_DEV_CTRL_SRST);
    wait(1);

    /*
     * We shall clear SRST in the Device Control register to 0.
     * And We shall wait over 2ms.
     * NOTE: We incidentally set interrupt disable.
     */
    ata_write(ch, ATA_REG_DEV_CTRL, ATA_FIELD_DEV_CTRL_N_IEN);
    wait(2);

    /* After wait, check busy bit. */
    ata_wait_status(ATA_PRIMARY, ATA_REG_STATUS_BSY, 0);
}


static inline uint8_t ata_do_cmd(uint8_t ch, uint8_t cmd) {
    ata_write(ch, ATA_REG_COMMAND, cmd);
    ata_wait400ns(ch);
    ata_wait_status(ch, ATA_REG_STATUS_BSY, 0);

    return ata_read(ch, ATA_REG_STATUS);
}


/* Do device selection protocol. */
static inline void ata_select_device(uint8_t ch,  uint8_t dev, uint8_t bits) {
    ata_wait_status(ch, ATA_REG_STATUS_BSY | ATA_REG_STATUS_DRQ, 0);

    /* Select Drive */
    ata_write(ch, ATA_REG_DEV_SELECT, (uint8_t)(dev << 4u) | bits);

    ata_wait400ns(ch);
    ata_wait_status(ch, ATA_REG_STATUS_BSY | ATA_REG_STATUS_DRQ, 0);
}


static inline uint8_t ata_access(uint8_t const direction, uint8_t const ch, uint8_t const dev, uint32_t const lba, uint8_t const sector_cnt, uint8_t* buffer) {
    uint8_t addr_mode;
    uint8_t lba_io[6];
    uint8_t head, sector;
    uint16_t cylinder;

    Dev const* d = &atas[ch].dev[dev];
    if (d->is_exists == 0 || d->type == TYPE_UNKNOWN || d->sector_nr <= (lba + sector_cnt)) {
        /* error */
        return ATA_ERROR;
    }

    /* Select addressing LBA28, LBA48 or CHS */
    if (0x10000000 <= lba) {
        /*
         * LBA48
         * But, max is 32 bit
         */
        addr_mode = 2;
        lba_io[0] = (lba & 0x000000FFu) >> 0u;
        lba_io[1] = (lba & 0x0000FF00u) >> 8u;
        lba_io[2] = (lba & 0x00FF0000u) >> 16u;
        lba_io[3] = (lba & 0xFF000000u) >> 24u;
        lba_io[4] = 0;
        lba_io[5] = 0;
        head = 0;
    } else if (atas[ch].dev[dev].capabilities & ATA_CAPABILITIES_LBA) {
        /* LBA28 */
        addr_mode = 1;
        lba_io[0] = (lba & 0x00000FF) >> 0;
        lba_io[1] = (lba & 0x000FF00) >> 8;
        lba_io[2] = (lba & 0x0FF0000) >> 16;
        lba_io[3] = 0;
        lba_io[4] = 0;
        lba_io[5] = 0;
        head = (lba & 0xF000000) >> 24;
    } else {
        /* CHS */
        addr_mode = 0;
        sector = (lba % 63) + 1;
        cylinder = ECAST_UINT16((lba + 1 - sector) / (16 * 63));
        lba_io[0] = sector;
        lba_io[1] = (cylinder >> 0) & 0xFF;
        lba_io[2] = (cylinder >> 8) & 0xFF;
        lba_io[3] = 0;
        lba_io[4] = 0;
        lba_io[5] = 0;
        head = (lba + 1 - sector) % (16 * 63) / (63);
        /* Head number is written to ATA_REG_DEV_CTRL lower 4-bits. */
    }

    /* TODO: See if drive supports DMA or not */
    uint8_t dma = 0;

    /* Select Drive and set LBA or CHS */
    ata_select_device(ch, dev, head | (addr_mode == 0 ? 0 : 0x40));

    /* Write adress. */
    if (addr_mode == 2) {
        /*
         * In a device implementing the 48-bit Address feature set, the Features register, the Sector Count register,
         * the LBA Low register, the LBA Mid register, and the LBA High register are each a two byte deep FIFO.
         */
        ata_write(ch, ATA_REG_SECTOR_COUNT, 0);
        ata_write(ch, ATA_REG_LBA0, lba_io[3]);
        ata_write(ch, ATA_REG_LBA1, lba_io[4]);
        ata_write(ch, ATA_REG_LBA2, lba_io[5]);
    }
    ata_write(ch, ATA_REG_SECTOR_COUNT, sector_cnt);
    ata_write(ch, ATA_REG_LBA0, lba_io[0]);
    ata_write(ch, ATA_REG_LBA1, lba_io[1]);
    ata_write(ch, ATA_REG_LBA2, lba_io[2]);

    uint8_t cmd;
    if (addr_mode == 0 && dma == 0 && direction == ATA_READ) { cmd = ATA_CMD_READ_PIO; }
    if (addr_mode == 1 && dma == 0 && direction == ATA_READ) { cmd = ATA_CMD_READ_PIO; }
    if (addr_mode == 2 && dma == 0 && direction == ATA_READ) { cmd = ATA_CMD_READ_PIO_EXT; }
    if (addr_mode == 0 && dma == 1 && direction == ATA_READ) { cmd = ATA_CMD_READ_DMA; }
    if (addr_mode == 1 && dma == 1 && direction == ATA_READ) { cmd = ATA_CMD_READ_DMA; }
    if (addr_mode == 2 && dma == 1 && direction == ATA_READ) { cmd = ATA_CMD_READ_DMA_EXT; }
    if (addr_mode == 0 && dma == 0 && direction == ATA_WRITE) { cmd = ATA_CMD_WRITE_PIO; }
    if (addr_mode == 1 && dma == 0 && direction == ATA_WRITE) { cmd = ATA_CMD_WRITE_PIO; }
    if (addr_mode == 2 && dma == 0 && direction == ATA_WRITE) { cmd = ATA_CMD_WRITE_PIO_EXT; }
    if (addr_mode == 0 && dma == 1 && direction == ATA_WRITE) { cmd = ATA_CMD_WRITE_DMA; }
    if (addr_mode == 1 && dma == 1 && direction == ATA_WRITE) { cmd = ATA_CMD_WRITE_DMA; }
    if (addr_mode == 2 && dma == 1 && direction == ATA_WRITE) { cmd = ATA_CMD_WRITE_DMA_EXT; }
    /* Wait and some error check is in for loop below. */
    ata_write(ch, ATA_REG_COMMAND, cmd);

    /* DMA operation. */
    if (dma != 0) {
        if (direction == ATA_READ) {
            /* Read. */
        } else {
            /* Write. */
        }

        return 0;
    }

    /* PIO operation. */
    uint32_t sec_size = d->sector_size;
    uint32_t sec_word_size = d->sector_size / 2;
    uint8_t error = 0;
    uint16_t port = get_port_number(ch, ATA_REG_DATA);
    if (direction == ATA_READ) {
        /* Read. */
        for (uint8_t i = 0; i < sector_cnt; i++) {
            /* Polling */
            error = ata_polling(ch, true);
            if (error != 0) {
                return error;
            }

            /* Read 1 sector. */
            __asm__ volatile(
                    "cld        \n"
                    "rep insw   \n"
                    :
                    : "d"(port), "D"(buffer), "c"(sec_word_size)
                    : "memory", "%ecx", "%edx", "%edi");

            /* Shift read size. */
            buffer += (sec_size);
        }
    } else {
        /* Write. */
        for (uint8_t i = 0; i < sector_cnt; i++) {
            error = ata_polling(ch, true);
            if (error != 0) {
                return error;
            }

            /* Read 1 sector. */
            __asm__ volatile(
                    "cld        \n"
                    "rep outsw  \n"
                    :
                    : "d"(port), "S"(buffer), "c"(sec_word_size)
                    : "memory", "%ecx", "%edx", "%esi");

            /* Shift write size. */
            buffer += (sec_size);
        }
        ata_write(ch, ATA_REG_COMMAND, (uint8_t[]) { ATA_CMD_CACHE_FLUSH, ATA_CMD_CACHE_FLUSH, ATA_CMD_CACHE_FLUSH_EXT }[addr_mode]);
        error = ata_polling(ch, false);
        if (error != 0) {
            return error;
        }
    }

    return 0;
}


Axel_state_code init_ata(void) {
    Pci_conf_reg pcr = find_pci_dev(PCI_CLASS_MASS_STORAGE, PCI_SUBCLASS_IDE);

    /* Read IRQ line. */
    Pci_data_reg pdr = pci_read_addr_data(&pcr, 0x3c);
    pci_write_data(0xfe);
    pdr = pci_read_data(pcr);

    if ((pdr.h0.reg_3c.interrupt_line == 0) && (pdr.h0.reg_3c.interrupt_pin == 0)) {
        /* This is a Parallel IDE Controller which uses IRQs 14 and 15. */
        /* printf("IRQ is 14 and 15\n"); */
    } else {
        /* This device needs an IRQ assignment. */
        return AXEL_FAILED;
    }

    uint16_t base_addrs[5] = {
        ECAST_UINT16(pci_read_addr_data(&pcr, 0x10).h0.reg_10.base_addr0), ECAST_UINT16(pci_read_addr_data(&pcr, 0x14).h0.reg_14.base_addr1), ECAST_UINT16(pci_read_addr_data(&pcr, 0x18).h0.reg_18.base_addr2), ECAST_UINT16(pci_read_addr_data(&pcr, 0x1c).h0.reg_1c.base_addr3), ECAST_UINT16(pci_read_addr_data(&pcr, 0x20).h0.reg_20.base_addr4),
    };

    for (size_t i = 0; i < ARRAY_SIZE_OF(base_addrs) - 1; i++) {
        if (!(base_addrs[i] == 0 || base_addrs[i] == 1)) {
            return AXEL_FAILED;
        }
    }

    /* Detect I/O Ports which interface IDE Controller: */
    atas[ATA_PRIMARY].command_base = (base_addrs[0] & 0xFFFC) + (0x1F0 * ((base_addrs[0] == 0) ? 1 : 0));
    atas[ATA_PRIMARY].control_base = (base_addrs[1] & 0xFFFC) + (0x3F6 * ((base_addrs[1] == 0) ? 1 : 0));
    atas[ATA_PRIMARY].bus_master = (base_addrs[4] & 0xFFFC) + 0;
    atas[ATA_SECONDARY].command_base = (base_addrs[2] & 0xFFFC) + (0x170 * ((base_addrs[2] == 0) ? 1 : 0));
    atas[ATA_SECONDARY].control_base = (base_addrs[3] & 0xFFFC) + (0x376 * ((base_addrs[3] == 0) ? 1 : 0));
    atas[ATA_SECONDARY].bus_master = (base_addrs[4] & 0xFFFC) + 8;

    ata_wait_status(ATA_PRIMARY, ATA_REG_STATUS_BSY, 0);
    ata_wait_status(ATA_SECONDARY, ATA_REG_STATUS_BSY, 0);

    ata_dev_reset(ATA_PRIMARY);
    ata_dev_reset(ATA_SECONDARY);

    for (uint8_t ch = 0; ch < 2; ch++) {
        Ata* ata = &atas[ch];
        for (uint8_t dev = 0; dev < 2; dev++) {
            Dev* d = &ata->dev[dev];

            /* Select drive.  */
            ata_select_device(ch, dev, 0);

            /* Do identify command and check device */
            if (ata_do_cmd(ch, ATA_CMD_IDENTIFY) == 0) {
                /* device NOT found. */
                d->is_exists = 0;
                continue;
            }
            d->is_exists = 1;

            /* Check type */
            uint8_t result = ata_polling(ch, true);
            if (result != 0) {
                /* Error occur, NOT ATA device, It is ATAPI. */
                d->type = TYPE_ATAPI;
            } else {
                /* ATA device prepared data transfer. */
                d->type = TYPE_ATA;
            }

            /* ATAPI identify command is difference. */
            if (d->type == TYPE_ATAPI) {
                /*
                 * Check signature for devices implementing the PACKET command feature set
                 * And reserved signatures for Serial ATA Working Groups.
                 */
                uint8_t bcl = ata_read(ch, ATA_REG_BCNT_LOW);
                uint8_t bch = ata_read(ch, ATA_REG_BCNT_HIGH);
                if (!(bcl == 0x14 && bch == 0xEB) && !(bcl == 0x69 && bch == 0x96)) {
                    /* Unknown Type device (may not be a device). */
                    d->is_exists = 0;
                    continue;
                }

                /* Do identify packet command. */
                ata_do_cmd(ch, ATA_CMD_IDENTIFY_PACKET);

                uint8_t result = ata_polling(ch, true);
                if (result != 0) {
                    d->type = TYPE_UNKNOWN;
                    continue;
                }
            }

            /* Read identify command result. */
            __asm__ volatile(
                    "cld        \n"
                    "rep insw   \n"
                    :
                    : "d"(get_port_number(ch, ATA_REG_DATA)), "D"(id_buffer), "c"(ATA_IDENTIFY_DATA_WORD_SIZE)
                    : "memory", "%ecx", "%edx", "%edi");

            /* Get device information. */
            d->gen_conf = id_buffer[ATA_IDENTIFY_GEN_CONF];
            d->capabilities = id_buffer[ATA_IDENTIFY_CAPABILITIES];
            memcpy(&d->command_sets, &id_buffer[ATA_IDENTIFY_CMD_SET0], sizeof(uint32_t));

            if ((d->command_sets & ATA_CMD_SET_LBA48) != 0) {
                /* Device supported 48-Bit Addressing. */
                memcpy(&d->sector_nr, &id_buffer[ATA_IDENTIFY_LBA48_MAX0], sizeof(uint32_t));
            } else {
                /* Device supported CHS or 28-bit Addressing: */
                memcpy(&d->sector_nr, &id_buffer[ATA_IDENTIFY_SECTOR_NR0], sizeof(uint32_t));
            }

            // char model[40 + 1];
            // model[40] = '\0';
            // memcpy(model, &id_buffer[27], 40);
            // printf("%s\n", model);

            uint16_t ssize = id_buffer[ATA_IDENTIFY_SECTOR_SIZE];
            if (((ssize & 0x4000) == 1) && ((ssize & 0x8000) == 0)) {
                /* Valid info. */
                if ((ssize & 0x1000) == 1) {
                    /* The device has been formatted with a logical sector size larger than 256 words. */
                    uint32_t lsector_wsize;
                    memcpy(&lsector_wsize, &id_buffer[ATA_IDENTIFY_SECTOR_SIZE], sizeof(uint32_t));
                    /* this field contain word size. */
                    d->sector_size = lsector_wsize * 2;
                } else {
                    d->sector_size = 512;
                }
            } else {
                d->sector_size = 512;
            }
        }
    }

    return AXEL_SUCCESS;
}
