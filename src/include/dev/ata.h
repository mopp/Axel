/**
 * @file ata.h
 * @brief ATA/ATAPI header.
 * @author mopp
 * @version 0.1
 * @date 2014-10-20
 */

#ifndef _ATA_H_
#define _ATA_H_



#include <state_code.h>
#include <stdint.h>
#include <stddef.h>


enum ata_constants {
    TYPE_ATA         = 0x00,
    TYPE_ATAPI       = 0x01,
    TYPE_UNKNOWN     = 0x02,

    ATA_PRIMARY      = 0x00,
    ATA_SECONDARY    = 0x01,

    ATA_MASTER       = 0x00,
    ATA_SLAVE        = 0x01,

    ATA_READ         = 0x00,
    ATA_WRITE        = 0x01,

    ATA_MAX_DRIVE_NR = 4,
};


struct ata_dev {
    uint8_t is_exists;     /* 0 (Empty) or 1 (This Drive really exists). */
    uint8_t type;          /* ATA, ATAPI or Unknown. */
    uint8_t channel;       /* Primary or Secondary. */
    uint8_t ms;            /* Master or Slave. */
    uint16_t gen_conf;     /* General configuration bit-significant information: */
    uint16_t capabilities; /* Features. */
    uint32_t command_sets; /* Command Sets Supported. */
    uint32_t sector_nr;    /* How many sectors. */
    uint32_t sector_size;  /* Byte size per 1 sector. */
};
typedef struct ata_dev Ata_dev;



extern Axel_state_code init_ata(void);
extern Ata_dev* get_ata_device(uint8_t);
extern Axel_state_code ata_access(uint8_t const, Ata_dev*, uint32_t, uint8_t, uint8_t*);
extern size_t get_ata_device_size(Ata_dev const*);



#endif
