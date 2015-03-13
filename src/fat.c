/**
 * @file fat.c
 * @brief fat file system implementation.
 *        NOTE: "Sector number" means that relative number based on sector 0(first sector of any volume).
 *        And "Physical sector number" means that absolute number based on head of disk.
 * @author mopp
 * @version 0.1
 * @date 2014-10-25
 */


#include <dev/ata.h>
#include <kernel.h>
#include <stdint.h>
#include <fat.h>
#include <utils.h>
#include <macros.h>
#include <paging.h>


static inline Axel_state_code access(void* p, uint8_t direction, uint32_t lba, uint8_t sec_cnt, uint8_t* buf) {
    Fat_manager* ft = p;
    uint8_t d = (direction == FILE_READ) ? ATA_READ : ATA_WRITE;
    return ata_access(d, ft->super.dev, ft->super.pe.lba_first + lba, sec_cnt, buf);
}


static File* fetch_child_directory(File* f) {
    if (f == NULL) {
        return NULL;
    }
    Fat_file* ff = (Fat_file*)f;


    if (ff->super.state_load == 1) {
        /* Already loaded. */
        return f;
    }

    Fat_manager* const fm = (Fat_manager*)ff->super.belong_fs;
    Bios_param_block* const bpb = fm->bpb;

    if (ff->super.type_dir == 0) {
        return NULL;
    }

    /* Traverse all chain FAT data cluster and read its structure. */
    Fat_file* ffiles = kmalloc(512 * sizeof(Fat_file));
    size_t file_cnt = 0;
    uint32_t limit = (bpb->sec_per_clus * bpb->bytes_per_sec) / sizeof(Dir_entry);
    uint32_t fe = 0;
    uint32_t next_clus = ff->clus_num;
    uint32_t clus;
    do {
        clus = next_clus;
        if ((fm->manip.fat_type == FAT_TYPE32) || ((uintptr_t)ff != (uintptr_t)fm->super.root_dir)) {
            /* ff is FAT32 or not root entry of FAT12/16.  */
            fe = fat_entry_access(&fm->manip, FILE_READ, clus, 0);
            if (is_valid_data_exist_fat_entry(&fm->manip, fe) == false) {
                /* Invalid */
                kfree(ffiles);
                return NULL;
            }

            /* FAT entry indicates last entry or next entry cluster. */
            next_clus = fe;

            /* Read entry. */
            if (fat_data_cluster_access(&fm->manip, FILE_READ, clus, fm->buffer) == NULL) {
                /* Invalid */
                kfree(ffiles);
                return NULL;
            }
        } else {
            /* ff is root entry of FAT12/16. */

            /* Set dummy entry data */
            switch (fm->manip.fat_type) {
                case FAT_TYPE12:
                    fe = 0xFF8;
                    break;
                case FAT_TYPE16:
                    fe = 0xFFF8;
                    break;
            }

            /* Read from root directory area. */
            access(&fm->manip, FILE_READ, fm->manip.area.rdentry.begin_sec, 1, fm->buffer);
        }

        Dir_entry* const de = (Dir_entry*)(uintptr_t)fm->buffer;
        Long_dir_entry* const lde = (Long_dir_entry*)(uintptr_t)fm->buffer;

        /* Find children */
        for (uint32_t i = 0; i < limit; i++) {
            Dir_entry* entry = &de[i];

            if (entry->name[0] == DIR_LAST_FREE) {
                break;
            }

            if (entry->name[0] == DIR_FREE || is_lfn(entry) == true) {
                /* Free and LFN entry is skiped. */
                continue;
            }

            /* This Entry is SFN entry. */
            uint8_t checksum = fat_calc_checksum(entry);
            bool is_error = false;

            /* Set name */
            char name[FAT_FILE_MAX_NAME_LEN];
            if (is_lfn(&de[i - 1]) == true) {
                /* There are LFN before SFN. */
                uint32_t j = i - 1;
                size_t index = 0;
                do {
                    if (checksum != lde[j].checksum) {
                        is_error = true;
                        break;
                    }
                    fat_get_long_dir_name(&lde[j], name, &index);
                } while (j != 0 && (lde[j--].order & LFN_LAST_ENTRY_FLAG) == 0 && index < 102);
            } else {
                /* SFN stand alone. */
                memcpy(name, entry->name, 11);
                name[11] = '\0';
            }
            if (is_error == true) {
                continue;
            }

            trim_tail(name);
            size_t len = strlen(name);

            Fat_file* f = &ffiles[file_cnt];
            f->super.name = kmalloc_zeroed(sizeof(uint8_t) * (len + 1));

            strcpy(f->super.name, name);

            /* Set the others. */
            f->clus_num = (uint16_t)((entry->first_clus_num_hi << 16) | entry->first_clus_num_lo);
            f->super.type = ((entry->attr & DIR_ATTR_DIRECTORY) != 0) ? FILE_TYPE_DIR : FILE_TYPE_FILE;
            f->super.size = entry->file_size;
            f->super.belong_fs = &fm->super;
            f->super.parent_dir = &ff->super;
            f->super.state_load = 0;

            file_cnt++;
        }
    } while (is_last_fat_entry(&fm->manip, fe) == false);

    if (file_cnt != 0) {
        /* Store file infomation */
        ff->super.children = kmalloc_zeroed(sizeof(File*) * file_cnt);
        ff->super.child_nr = file_cnt;
        ff->super.state_load = 1;
        for (size_t i = 0; i < file_cnt; i++) {
            ff->super.children[i] = kmalloc_zeroed(sizeof(Fat_file));
            memcpy(ff->super.children[i], &ffiles[i], sizeof(Fat_file));
        }
    }
    kfree(ffiles);

    return &ff->super;
}


static inline Axel_state_code fat_access_file(uint8_t direction, File const* const f, uint8_t* const buffer) {
    if (f == NULL || buffer == NULL || f->type_file == 0 || f->size == 0) {
        return AXEL_FAILED;
    }

    Fat_file const* const ff = (Fat_file const* const)f;
    Fat_manager* const fm = (Fat_manager*)f->belong_fs;

    /* Allocate buffer that have enough size for reading/writing per cluster unit. */
    uint32_t const bpc = (fm->bpb->sec_per_clus * fm->bpb->bytes_per_sec);
    uint32_t const rounded_size = (uint32_t)((f->size + bpc - 1) / bpc) * bpc;
    uint8_t* const ebuffer = kmalloc(sizeof(uint8_t) * rounded_size);
    if (ebuffer == NULL) {
        return AXEL_FAILED;
    }
    uint8_t* eb = ebuffer;
    uint8_t* const eb_limit = ebuffer + rounded_size;

    uint8_t rw;
    if (direction == FILE_WRITE) {
        rw = FILE_WRITE;
        /* Copy data to write */
        memcpy(eb, buffer, f->size);
    } else {
        rw = FILE_READ;
    }

    uint32_t fe, clus, next_clus;
    next_clus = ff->clus_num;
    do {
        clus = next_clus;
        fe = fat_entry_access(&fm->manip, FILE_READ, clus, 0);

        if (is_valid_data_exist_fat_entry(&fm->manip, fe) == false) {
            /* Invalid. */
            goto failed;
        }

        /* FAT entry indicates last entry or next entry cluster. */
        next_clus = fe;

        /* Access data. */
        if (fat_data_cluster_access(&fm->manip, rw, clus, eb) != eb) {
            goto failed;
        }

        /* Shift Next cluster. */
        eb += bpc;
    } while (is_last_fat_entry(&fm->manip, fe) == false);

    if (rw == FILE_READ && eb < eb_limit) {
        /* error */
        goto failed;
    }

    if (rw == FILE_WRITE && eb < eb_limit) {
        /* Allocate new cluster because file size becomes bigger. */
        uint32_t alloc_clus;
        do {
            alloc_clus = alloc_cluster(&fm->manip);

            /* Set FAT chain. */
            if (alloc_clus == 0 || fat_entry_access(&fm->manip, FILE_WRITE, clus, alloc_clus) == 0) {
                /* allocate failed */
                goto failed;
            }

            /* Write data. */
            if (fat_data_cluster_access(&fm->manip, rw, clus, eb) != eb) {
                goto failed;
            }

            /* Shift Next cluster. */
            eb += bpc;
        } while (eb < eb_limit);

        /* Set chain tail. */
        if (set_last_fat_entry(&fm->manip, clus) == 0) {
            goto failed;
        }
    }

    if (direction == FILE_READ) {
        memcpy(buffer, ebuffer, f->size);
    }


    kfree(ebuffer);
    return AXEL_SUCCESS;

failed:
    kfree(ebuffer);
    return AXEL_FAILED;
}


static inline Axel_state_code fat_change_dir(File_system* ft, char const* path) {
    if (ft == NULL || path == NULL || ft->current_dir == NULL) {
        return AXEL_FAILED;
    }

    File_system const stored_ft = *ft;

    /* Absolute path. */
    while (*path == '/') {
        path++;
        ft->current_dir = ft->root_dir;
        if (*path == '\0') {
            return AXEL_SUCCESS;
        }
    }

    bool is_change = false;
    char const* c;
    do {
        Fat_file const* ff = (Fat_file*)ft->current_dir;
        if ((path[0] == '.') && (path[1] == '.')) {
            /* Go to parent dir. */
            if (ft->current_dir == ft->root_dir) {
                goto failed;
            }
            ft->current_dir = ff->super.parent_dir;
            path += 2;
            is_change = true;
        } else {
            /* Search sub directory. */
            c = strchr(path, '/');
            size_t len = ((c == NULL) ? strlen(path) : ((uintptr_t)c - (uintptr_t)path));
            size_t cnr = ff->super.child_nr;
            for (size_t i = 0; i < cnr; i++) {
                Fat_file* ffi = (Fat_file*)ff->super.children[i];
                /* Checking directory && Not current directory && match name. */
                if ((ffi->super.type_dir == 1) && (&ffi->super != ft->current_dir) && (memcmp(path, ffi->super.name, len) == 0)) {
                    ft->current_dir = &ffi->super;
                    is_change = true;

                    /* Read sub directory contents. */
                    if (ft->current_dir->state_load == 0 && fetch_child_directory(&ffi->super) == NULL) {
                        goto failed;
                    }
                    break;
                }
            }
            path += len;
        }

        if (*path == '/') {
            path++;
        }
    } while (*path != '\0');


    return (is_change == true) ? AXEL_SUCCESS : AXEL_FAILED;

failed:
    *ft = stored_ft;
    return AXEL_FAILED;
}


/* LBA = (H + C * Total Head size) * (Sector size per track) + (S - 1) */
File_system* init_fat(Ata_dev* dev, Partition_entry* pe) {
    if (dev == NULL || pe == NULL) {
        return NULL;
    }

    Fat_manager* const fm = kmalloc_zeroed(sizeof(Fat_manager));
    fm->super.dev         = dev;
    fm->super.pe          = *pe;
    fm->super.change_dir  = fat_change_dir;
    fm->super.access_file = fat_access_file;
    fm->super.fetch_child_directory = fetch_child_directory;

    Fat_file* const root  = kmalloc_zeroed(sizeof(Fat_file));
    root->super.name      = kmalloc(1 + 1);
    root->super.belong_fs = &fm->super;
    root->super.type      = FILE_TYPE_DIR | FILE_TYPE_ROOT;
    fm->super.current_dir = &root->super;
    fm->super.root_dir    = &root->super;
    strcpy(root->super.name, "/");

    /* Load Bios parameter block. */
    Bios_param_block* const bpb = kmalloc(sizeof(Bios_param_block));
    fm->bpb = bpb;
    size_t const bytes_per_sec = dev->sector_size;
    uint8_t* const b = kmalloc(bytes_per_sec);
    if ((access(fm, FILE_READ, 0, 1, (uint8_t*)b) != AXEL_SUCCESS) || (*((uint16_t*)(uintptr_t)(&b[510])) == 0x55aa)) {
        goto failed;
    }
    memcpy(bpb, b, sizeof(Bios_param_block));

    /* Free temporary buffer and allocate buffer of one cluster. */
    kfree(b);
    fm->buffer_size = bpb->sec_per_clus * bytes_per_sec;
    fm->buffer = kmalloc(fm->buffer_size);

    fm->manip.obj          = fm;
    fm->manip.bpb          = bpb;
    fm->manip.b_access     = access;
    fm->manip.alloc        = kmalloc;
    fm->manip.free         = kfree;
    fm->manip.byte_per_cluster = bpb->sec_per_clus * bpb->bytes_per_sec;
    fm->manip.fsinfo = kmalloc(sizeof(Fsinfo));

    fat_calc_sectors(bpb, &fm->manip.area);

    /* FAT type detection. */
    fm->cluster_nr = fm->manip.area.data.sec_nr / bpb->sec_per_clus;
    if (fm->cluster_nr < 4085) {
        fm->manip.fat_type = FAT_TYPE12;
        root->clus_num = 0;
    } else if (fm->cluster_nr < 65525) {
        fm->manip.fat_type = FAT_TYPE16;
        root->clus_num = 0;
    } else {
        fm->manip.fat_type = FAT_TYPE32;
        if (fat_fsinfo_access(&fm->manip, FILE_READ, fm->manip.fsinfo) == NULL) {
            goto failed;
        }

        if (fm->manip.fsinfo->free_cnt == FSINFO_INVALID_FREE) {
            /* TODO: count free cluster */
        }

        if (fm->manip.fsinfo->next_free == FSINFO_INVALID_FREE) {
            /* TODO: set last allocated cluster number. */
        }

        root->clus_num = bpb->fat32.root_dentry_cluster;
    }

    /* Load root directory entry. */
    if (fetch_child_directory(&root->super) == NULL) {
        goto failed;
    }

    return &fm->super;

    failed:
    kfree(fm);
    kfree(fm->buffer);
    kfree(root);
    return NULL;
}
