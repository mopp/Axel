/**
 * @file fs.c
 * @brief file system implementation.
 * @author mopp
 * @version 0.1
 * @date 2014-10-25
 */


#include <fs.h>
#include <fat.h>
#include <stdint.h>
#include <dev/ata.h>
#include <kernel.h>
#include <paging.h>
#include <macros.h>


static Master_boot_record* mbr;


File_system* init_fs(Ata_dev* dev) {
    if (dev == NULL || dev->is_exists == 0) {
        return NULL;
    }

    /* Load first sector. */
    uint8_t* first_sec = kmalloc(axel_s.main_disk->sector_size);
    if (ata_access(ATA_READ, axel_s.main_disk, 0, 1, first_sec) != AXEL_SUCCESS) {
        return NULL;
    }
    mbr = (Master_boot_record*)first_sec;

    if (mbr->boot_signature != BOOT_SIGNATURE) {
        /* Invalid MBR. */
        return NULL;
    }

    /* Check file system. */
    Partition_entry* pe;
    for (uint8_t i = 0; i < 4; i++) {
        pe = &mbr->p_entry[i];
        if (pe->type == PART_TYPE_FAT32 || pe->type == PART_TYPE_FAT32_LBA) {
            File_system* fs = kmalloc(sizeof(File_system));
            fs->dev = dev;
            fs->pe = pe;
            return (init_fat(fs) == AXEL_SUCCESS) ? fs : NULL;
        }
    }

    return NULL;
}
