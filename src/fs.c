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
#include <utils.h>


static Master_boot_record* mbr;


File_system* init_fs(Ata_dev* dev) {
    if (dev == NULL || dev->is_exists == 0) {
        return NULL;
    }

    /* Load first sector. */
    uint8_t* first_sec = kmalloc(axel_s.main_disk->sector_size);
    if (first_sec == NULL || ata_access(ATA_READ, axel_s.main_disk, 0, 1, first_sec) != AXEL_SUCCESS) {
        kfree(first_sec);
        return NULL;
    }
    mbr = (Master_boot_record*)first_sec;

    if (mbr->boot_signature != BOOT_SIGNATURE) {
        /* Invalid MBR. */
        kfree(first_sec);
        return NULL;
    }

    /* Check file system. */
    Partition_entry* pe;
    File_system* fs = NULL;
    for (uint8_t i = 0; i < 4; i++) {
        pe = &mbr->p_entry[i];
        switch (pe->type) {
            case PART_TYPE_FAT32:
            case PART_TYPE_FAT32_LBA:
            case PART_TYPE_FAT16:
            case PART_TYPE_FAT12:
                fs = init_fat(dev, pe);
                break;
        }
    }
    kfree(first_sec);

    return fs;
}


/* TODO: stack */
File* resolve_path(File_system const* fs, char const* path) {
    if ((path[0] == '/') && (strstr(path, "..") == NULL)) {
        /* Absolute path. */
    }

    File* current = fs->current_dir;

    do {
        if (memcmp(path, "../", 3) == 0) {
            if (current == current->belong_fs->root_dir) {
                /* Path error. */
                return NULL;
            }
            /* Move to upper dir. */
            current = current->parent_dir;
            continue;
        }

        char* c = strchr(path, '/');
        size_t l = (c == NULL) ? (strlen(path)) : ((uintptr_t)c - (uintptr_t)path);
        size_t cnr = current->child_nr;
        for (size_t i = 0; i < cnr; i++) {
            File* child = current->children[i];
            if (memcmp(child->name, path, l) == 0) {
                path += (l + 1);
                current = child;
                break;
            }
        }
    } while ((path != '\0') && (current->type != FILE_TYPE_FILE));

    return current;
}
