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


struct file_system;
typedef struct file_system File_system;


/* Filesystem access interfaces. */
typedef Axel_state_code (*Fs_change_dir_func)(File_system* ft, char const* path);


struct file {
    union {
        uint8_t type;
        struct {
            uint8_t type_root : 1;
            uint8_t type_dir : 1;
            uint8_t type_file : 1;
            uint8_t type_dev : 1;
        };
    };
    union {
        uint8_t state;
        struct {
            uint8_t state_load;
        };
    };
    uint8_t lock;
    uint8_t permission;
    uint8_t* name;
    size_t size;
    size_t create_time;
    size_t write_time;
    struct file* parent_dir;
    struct file** children;
    size_t child_nr;
    struct file_system* belong_fs;
};
typedef struct file File;


struct file_system {
    Ata_dev* dev;
    Partition_entry pe;
    File* current_dir;
    File* root_dir;
    Fs_change_dir_func change_dir;
};
typedef struct file_system File_system;


enum fs_constants {
    BOOT_SIGNATURE      = 0xAA55,
    PART_TYPE_EMPTY     = 0x00,
    PART_TYPE_FAT12     = 0x01,
    PART_TYPE_FAT16     = 0x04,
    PART_TYPE_HPFS      = 0x07,
    PART_TYPE_NTFS      = 0x07,
    PART_TYPE_EX_FAT    = 0x07,
    PART_TYPE_FAT32     = 0x0B,
    PART_TYPE_FAT32_LBA = 0x1B,
    FILE_TYPE_ROOT      = 0x01,
    FILE_TYPE_DIR       = 0x02,
    FILE_TYPE_FILE      = 0x04,
    FILE_TYPE_DEV       = 0x08,
    FILE_STATE_LOAD     = 0x01,
};


File_system* init_fs(Ata_dev*);



#endif
