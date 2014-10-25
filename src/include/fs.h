/**
 * @file fs.h
 * @brief file system header.
 * @author mopp
 * @version 0.1
 * @date 2014-10-25
 */


#ifndef _FS_H_
#define _FS_H_



#include <state_code.h>
#include <stdint.h>
#include <dev/ata.h>


struct partition_entry {
    uint8_t bootable;
    struct {
        uint8_t head;
        uint8_t sector; /* This includes upper 2bit of cylinder */
        uint8_t cylinder;
    } first;
    uint8_t type;
    struct {
        uint8_t head;
        uint8_t sector;
        uint8_t cylinder;
    } last;
    uint32_t lba_first;
    uint32_t sector_nr;
};
typedef struct partition_entry Partition_entry;
_Static_assert(sizeof(Partition_entry) == 16, "Partition_entry is NOT 16 byte.");


struct master_boot_record {
    uint8_t boot_code0[218];
    uint8_t zeros[2];
    uint8_t original_pdrive;
    struct {
        uint8_t seconds;
        uint8_t minutes;
        uint8_t hours;
    } timestamp;
    uint8_t boot_code1[216];
    uint32_t disk_signature32;
    uint16_t copy_protected;
    Partition_entry p_entry[4];
    uint16_t boot_signature;
} __attribute__((packed));
typedef struct master_boot_record Master_boot_record;
_Static_assert(sizeof(Master_boot_record) == 512, "Master_boot_record is NOT 512 byte.");


enum {
    BOOT_SIGNATURE      = 0xAA55,
    PART_TYPE_EMPTY     = 0x00,
    PART_TYPE_FAT12     = 0x01,
    PART_TYPE_FAT16     = 0x04,
    PART_TYPE_HPFS      = 0x07,
    PART_TYPE_NTFS      = 0x07,
    PART_TYPE_EX_FAT    = 0x07,
    PART_TYPE_FAT32     = 0x0B,
    PART_TYPE_FAT32_LBA = 0x1B,
};


struct file_system {
    Ata_dev* dev;
    Partition_entry* pe;
};
typedef struct file_system File_system;


File_system* init_fs(Ata_dev*);



#endif
