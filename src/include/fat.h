/**
 * @file fat.c
 * @brief fat file system implementation.
 * @author mopp
 * @version 0.1
 * @date 2014-10-25
 */


#ifndef _FAT_H_
#define _FAT_H_



#include <state_code.h>
#include <fs.h>


File_system* init_fat(Ata_dev* dev, Partition_entry* pe);



#endif

