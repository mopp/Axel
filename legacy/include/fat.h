/**
 * @file fat.c
 * @brief fat file system implementation.
 * @author mopp
 * @version 0.1
 * @date 2014-10-25
 */


#if !defined(_FAT_H_) && !defined(FOR_IMG_UTIL)
#define _FAT_H_



#include <state_code.h>
#include <fs.h>
#include <fat_manip.h>


struct fat_file {
    File super;
    uint32_t clus_num;
};
typedef struct fat_file Fat_file;


struct fat_manager;
typedef struct fat_manager Fat_manager;
typedef Axel_state_code (*Dev_access)(Fat_manager const*, uint8_t, uint32_t, uint8_t, uint8_t*);


struct fat_manager {
    File_system super;
    Fat_manips manip;
    Bios_param_block* bpb;
    uint8_t* buffer; /* this buffer has area that satisfy one cluster size. */
    size_t buffer_size;
    uint32_t cluster_nr;
};
typedef struct fat_manager Fat_manager;


File_system* init_fat(Ata_dev* dev, Partition_entry* pe);



#endif
