/**
 * @file fs.h
 * @brief file system header.
 * @author mopp
 * @version 0.1
 * @date 2014-10-25
 */


#ifndef _FS_CONSTANTS_H_
#define _FS_CONSTANTS_H_



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
    FILE_READ,
    FILE_WRITE,
};



#endif



#if !defined(_FS_H_) && !defined(FOR_IMG_UTIL)
#define _FS_H_



#include <state_code.h>
#include <stdint.h>
#include <dev/ata.h>
#include <mbr.h>


struct file_system;
typedef struct file_system File_system;
struct file;
typedef struct file File;

/* Filesystem access interfaces. */
typedef Axel_state_code (*Fs_change_dir_func)(File_system*, char const*);
typedef Axel_state_code (*Fs_access_file)(uint8_t, File const*, uint8_t*);
typedef File* (*Fs_fetch_child_directory)(File*);


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
    char* name;
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
    File* current_dir;
    File* root_dir;
    Fs_change_dir_func change_dir;
    Fs_access_file access_file;
    Fs_fetch_child_directory fetch_child_directory;
    Partition_entry pe;
};
typedef struct file_system File_system;


File_system* init_fs(Ata_dev*);
File* resolve_path(File_system const*, char const*);



#endif
