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


File* find_child_dir(char const* file_name, size_t const file_name_len, File* base_file) {
    if ((file_name == NULL) || (base_file == NULL)) {
        return NULL;
    }

    if (base_file->state_load == 0) {
        /* If children directory are not loaded, load children directory. */
        File_system* fs = base_file->belong_fs;
        fs->fetch_child_directory(base_file);
    }

    for (size_t i = 0; i < base_file->child_nr; i++) {
        if (memcmp(base_file->children[i]->name, file_name, file_name_len) == 0) {
            return base_file->children[i];
        }
    }

    return NULL;
}


static File* resolve_path_sub(File_system const* fs, char const* path, File* f) {
    char* c = strchr(path, '/');

    if (c != NULL) {
        /* Path is directory name. */
        if (memcmp(path, "..", 2) == 0) {
            f = f->parent_dir;
        } else {
            uintptr_t name_length = (uintptr_t)c - (uintptr_t)path;
            f = find_child_dir(path, name_length, f);
            if (f == NULL) {
                return NULL;
            }
            path += name_length + 1;
        }

        return resolve_path_sub(fs, path, f);
    }

    /* Path is filename. */
    return find_child_dir(path, strlen(path), f);
}

File* resolve_path(File_system const* fs, char const* path) {
    if ((fs == NULL) || (path == NULL)) {
        return NULL;
    }

    File* f = NULL;
    if (path[0] == '/') {
        /* Absolute path. */
        f = fs->root_dir;
        path++;
    } else {
        /* Related path. */
        f = fs->current_dir;
    }

    return resolve_path_sub(fs, path, f);
}
