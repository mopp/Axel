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


struct bios_param_block {
    uint8_t jmp_boot[3];        /* Jump instruction for boot. */
    uint8_t oem_name[8];        /* System that makes this partition. */
    uint16_t bytes_per_sec;     /* byte size of one sector. */
    uint8_t sec_per_clus;       /* The number of sector per cluster, this is power of 2. */
    uint16_t rsvd_area_sec_num; /* The number of sector of reserved area sector  */
    uint8_t num_fats;           /* The number of FAT. */
    uint16_t root_ent_cnt;      /* This is only used by FAT12 or FAT16. */
    uint16_t total_sec16;       /* This is only used by FAT12 or FAT16. */
    uint8_t media;              /* This is not used. */
    uint16_t fat_size16;        /* This is only used by FAT12 or FAT16. */
    uint16_t sec_per_trk;       /* The number of sector of a track. */
    uint16_t num_heads;         /* The number of head. */
    uint32_t hidd_sec;          /* The number of physical sector that is located on before this volume. */
    uint32_t total_sec32;       /* Total sector num of FAT32 */
    union {
        struct {
            uint8_t drive_num;
            uint8_t reserved_1;
            uint8_t boot_sig;
            uint32_t volume_id;
            uint8_t volume_label[11];
            uint8_t fs_type[8];
        } fat16;
        struct {
            uint16_t fat_size32; /* The number of sector of one FAT. */
            union {
                uint16_t ext_flags;
                struct {
                    uint8_t active_fat : 4; /* This is enable when mirror_all_fat is 1. */
                    uint8_t ext_reserved0 : 3;
                    uint8_t is_active : 1; /* if this is 0, all FAT is mirroring, otherwise one FAT is only enable. */
                    uint8_t ext_reserved1;
                };
            };
            uint16_t fat32_version;   /* FAT32 volume version. */
            uint32_t rde_clus_num;    /* First cluster number of root directory entry. */
            uint16_t fsinfo_sec_num;  /* Sector number of FSINFO struct location in reserved area(always 1). */
            uint16_t bk_boot_sec;     /* Backup of boot sector sector number. */
            uint8_t reserved[12];     /* Reserved. */
            uint8_t drive_num;        /* BIOS drive number. */
            uint8_t reserved1;        /* Reserved. */
            uint8_t boot_sig;         /* Extend boot signature. */
            uint32_t volume_id;       /* Volume serial number. */
            uint8_t volume_label[11]; /* Partition volume label */
            uint8_t fs_type[8];
        } fat32;
    };
} __attribute__((packed));
typedef struct bios_param_block Bios_param_block;


/*
 * NOTE: free_cnt and next_free is not necessary correct.
 */
struct fsinfo {
    uint32_t lead_signature;   /* This is used to validate that this is in fact an FSInfo sector. */
    uint8_t reserved0[480];    /* must be zeros. */
    uint32_t struct_signature; /* This is more localized in the sector to the location of the fields that are used. */
    uint32_t free_cnt;         /* The last known free cluster count */
    uint32_t next_free;        /* The cluster number at which the driver should start looking for free clusters. */
    uint8_t reserved1[12];     /* must be zeros. */
    uint32_t trail_signature;  /* This is used to validate that this is in fact an FSInfo sector. */
};
typedef struct fsinfo Fsinfo;
_Static_assert(sizeof(Fsinfo) == 512, "Fsinfo is NOT 512 byte.");


/* Short file name formant. */
struct dir_entry {
    uint8_t name[11]; /* short name. */
    uint8_t attr;     /* attribute. */
    uint8_t nt_rsvd;  /* reserved by Windows NT. */
    uint8_t create_time_tenth;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t last_access_date;
    uint16_t first_clus_num_hi;
    uint16_t write_time;
    uint16_t write_date;
    uint16_t first_clus_num_lo;
    uint32_t file_size;
};
typedef struct dir_entry Dir_entry;
_Static_assert(sizeof(Dir_entry) == 32, "Dir_entry is NOT 32 byte.");


/* Long file name formant. */
struct long_dir_entry {
    uint8_t order; /* The order of this entry in the sequence of long dir entries. */
    uint8_t name0[10];
    uint8_t attr;
    uint8_t type;
    uint8_t checksum;
    uint8_t name1[12];
    uint16_t first_clus_num_lo;
    uint8_t name2[4];
};
typedef struct long_dir_entry Long_dir_entry;
_Static_assert(sizeof(Long_dir_entry) == 32, "Long_dir_entry is NOT 32 byte.");


struct fat_file {
    File super;
    uint32_t clus_num;
};
typedef struct fat_file Fat_file;


struct fat_area {
    uint32_t begin_sec;
    uint32_t sec_nr;
};
typedef struct fat_area Fat_area;


struct fat_manager {
    File_system super;
    Bios_param_block bpb;
    Fsinfo fsinfo;
    uint8_t* buffer;
    size_t buffer_size;
    Fat_area rsvd;
    Fat_area fat;
    Fat_area rdentry;
    Fat_area data;
    uint32_t cluster_nr;
    uint8_t fat_type;
};
typedef struct fat_manager Fat_manager;


enum {
    FSINFO_LEAD_SIG         = 0x41615252,
    FSINFO_STRUCT_SIG       = 0x61417272,
    FSINFO_TRAIL_SIG        = 0xAA550000,
    FSINFO_INVALID_FREE     = 0xFFFFFFFF,
    DIR_LAST_FREE           = 0x00,
    DIR_FREE                = 0xe5,
    DIR_ATTR_READ_ONLY      = 0x01,
    DIR_ATTR_HIDDEN         = 0x02,
    DIR_ATTR_SYSTEM         = 0x04,
    DIR_ATTR_VOLUME_ID      = 0x08,
    DIR_ATTR_DIRECTORY      = 0x10,
    DIR_ATTR_ARCHIVE        = 0x20,
    DIR_ATTR_LONG_NAME      = DIR_ATTR_READ_ONLY | DIR_ATTR_HIDDEN | DIR_ATTR_SYSTEM | DIR_ATTR_VOLUME_ID,
    DIR_ATTR_LONG_NAME_MASK = DIR_ATTR_READ_ONLY | DIR_ATTR_HIDDEN | DIR_ATTR_SYSTEM | DIR_ATTR_VOLUME_ID | DIR_ATTR_DIRECTORY | DIR_ATTR_ARCHIVE,
    LFN_LAST_ENTRY_FLAG     = 0x40,
    FAT_TYPE12              = 12,
    FAT_TYPE16              = 16,
    FAT_TYPE32              = 32,
};


static inline Axel_state_code fat_access(uint8_t direction, Fat_manager const* ft, uint32_t lba, uint8_t sec_cnt, uint8_t* buf) {
    return ata_access((direction == FILE_READ) ? ATA_READ : ATA_WRITE, ft->super.dev, ft->super.pe.lba_first + lba, sec_cnt, buf);
}


static inline uint32_t fat_enrty_access(uint8_t direction, Fat_manager const* ft, uint32_t n, uint32_t write_entry) {
    uint32_t offset       = n * ((ft->fat_type == FAT_TYPE32) ? 4 : 2);
    uint32_t bps          = ft->bpb.bytes_per_sec;
    uint32_t sec_num      = ft->bpb.rsvd_area_sec_num + (offset / bps);
    uint32_t entry_offset = offset % bps;

    if (fat_access(FILE_READ, ft, sec_num, 1, ft->buffer) == AXEL_FAILED) {
        return 0;
    }
    uint32_t* b = (uint32_t*)(uintptr_t)&ft->buffer[entry_offset];
    uint32_t read_entry;

    /* Filter */
    switch (ft->fat_type) {
        case FAT_TYPE12:
            read_entry = (*b & 0xFFF);
            break;
        case FAT_TYPE16:
            read_entry = (*b & 0xFFFF);
            break;
        case FAT_TYPE32:
            read_entry = (*b & 0x0FFFFFFF);
            break;
    }

    if (direction == FILE_READ) {
        return read_entry;
    }

    /* Change FAT entry. */
    switch (ft->fat_type) {
        case FAT_TYPE12:
            *b = (write_entry & 0xFFF);
            break;
        case FAT_TYPE16:
            *b = (write_entry & 0xFFFF);
            break;
        case FAT_TYPE32:
            /* keep upper bits */
            *b = (*b & 0xF0000000) | (write_entry & 0x0FFFFFFF);
            break;
    }

    if (fat_access(FILE_WRITE, ft, sec_num, 1, ft->buffer) == AXEL_FAILED) {
        return 0;
    }

    return *b;
}


static inline Fsinfo* fsinfo_access(uint8_t direction, Fat_manager* fm) {
    if (direction == FILE_WRITE) {
        memcpy(fm->buffer, &fm->fsinfo, sizeof(Fsinfo));
    }

    /* Access FSINFO structure. */
    if (fat_access(direction, fm, fm->bpb.fat32.fsinfo_sec_num, 1, fm->buffer) == AXEL_FAILED) {
        return NULL;
    }

    if (direction == FILE_READ) {
        memcpy(&fm->fsinfo, fm->buffer, sizeof(Fsinfo));

        Fsinfo* fsinfo = &fm->fsinfo;
        if ((fsinfo->lead_signature != FSINFO_LEAD_SIG) || (fsinfo->struct_signature != FSINFO_STRUCT_SIG) || (fsinfo->trail_signature != FSINFO_TRAIL_SIG)) {
            /* Invalid. */
            return NULL;
        }
    }

    return &fm->fsinfo;
}


static inline uint8_t* fat_data_cluster_access(uint8_t direction, Fat_manager const * ft, uint32_t clus_num, uint8_t* buffer) {
    uint8_t spc = ft->bpb.sec_per_clus;
    uint32_t sec = ft->data.begin_sec + (clus_num - 2) * spc;
    if (fat_access(direction, ft, sec, spc, buffer) == AXEL_FAILED) {
        return NULL;
    }

    return buffer;
}


static inline bool is_unused_fat_entry(uint32_t fe) {
    return (fe == 0) ? true : false;
}


static inline bool is_rsvd_fat_entry(uint32_t fe) {
    return (fe == 1) ? true : false;
}


static inline bool is_bad_clus_fat_entry(Fat_manager const* fm, uint32_t fe) {
    switch (fm->fat_type) {
        case FAT_TYPE12:
            return (fe == 0xFF7) ? true : false;
        case FAT_TYPE16:
            return (fe == 0xFFF7) ? true : false;
        case FAT_TYPE32:
            return (fe == 0x0FFFFFF7) ? true : false;
    }

    return false;
}


static inline bool is_valid_data_fat_entry(Fat_manager const* fm, uint32_t fe) {
    /* NOT bad cluster and reserved. */
    return (is_bad_clus_fat_entry(fm, fe) == false && is_rsvd_fat_entry(fe) == false) ? true : false;
}


static inline bool is_data_exist_fat_entry(Fat_manager const* fm, uint32_t fe) {
    return (is_unused_fat_entry(fe) == false && is_valid_data_fat_entry(fm, fe) == true) ? true : false;
}


static inline bool is_last_fat_entry(Fat_manager const* fm, uint32_t fe) {
    switch (fm->fat_type) {
        case FAT_TYPE12:
            return (0xFF8 <= fe && fe <= 0xFFF) ? true : false;
        case FAT_TYPE16:
            return (0xFFF8 <= fe && fe <= 0xFFFF) ? true : false;
        case FAT_TYPE32:
            return (0x0FFFFFF8 <= fe && fe <= 0x0FFFFFFF) ? true : false;
    }

    /* ERROR. */
    return false;
}


static inline uint32_t set_last_fat_entry(Fat_manager const* fm, uint32_t clus) {
    uint32_t last;

    switch (fm->fat_type) {
        case FAT_TYPE12:
            last = 0xFFF;
        case FAT_TYPE16:
            last = 0xFFFF;
        case FAT_TYPE32:
            last = 0x0FFFFFFF;
    }

    return fat_enrty_access(FILE_WRITE, fm, clus, last);
}


static inline uint32_t alloc_cluster(Fat_manager* fm){
    if (fm->fsinfo.free_cnt == 0) {
        /* No free cluster. */
        return 0;
    }

    uint32_t begin = fm->fsinfo.next_free + 1;
    uint32_t end = fm->cluster_nr;
    uint32_t i, select_clus = 0;

    for (i = begin; i < end; i++) {
        uint32_t fe = fat_enrty_access(FILE_READ, fm, i, 0);
        if (is_unused_fat_entry(fe) == true) {
            /* Found unused cluster. */
            select_clus = i;
            break;
        }
    }

    if (select_clus == 0) {
        /* Unused cluster is not found. */
        return 0;
    }

    /* Update FSINFO. */
    fm->fsinfo.next_free = select_clus;
    fm->fsinfo.free_cnt -= 1;
    fsinfo_access(FILE_WRITE, fm);

    return select_clus;
}


static inline void free_cluster(Fat_manager* fm, uint32_t clus) {
    uint32_t fe;
    do {
        fe = fat_enrty_access(FILE_READ, fm, clus, 0);
        bool f = is_unused_fat_entry(fe);
        if (f == false || is_last_fat_entry(fm, fe) == true) {
            fat_enrty_access(FILE_WRITE, fm, clus, 0);
        }

        if (f == true) {
            return;
        }
        clus = fe;
    } while (1);
}


static inline bool is_lfn(Dir_entry const* de) {
    return ((de->attr & DIR_ATTR_LONG_NAME) == DIR_ATTR_LONG_NAME) ? true : false;
}


static inline size_t ucs2_to_ascii(uint8_t* ucs2, uint8_t* ascii, size_t ucs2_len) {
    if ((ucs2_len & 0x1) == 1) {
        /* invalid. */
        return 0;
    }

    size_t cnt = 0;
    for(size_t i = 0; i < ucs2_len; i++) {
        uint8_t c = ucs2[i];
        if((0 < c) && (c < 127)) {
            ascii[cnt++] = ucs2[i];
        }
    }
    ascii[cnt] = '\0';

    return cnt;
}


static inline uint8_t calc_checksum(Dir_entry const* de) {
    uint8_t sum = 0;

    for (uint8_t i = 0; i < 11; i++) {
        sum = (sum >> 1u) + (uint8_t)(sum << 7u) + de->name[i];
    }

    return sum;
}


static inline Fat_file* read_directory(Fat_file* ff) {
    if (ff->super.state_load == 1) {
        /* Already loaded. */
        return ff;
    }

    Fat_manager* const fm = (Fat_manager*)ff->super.belong_fs;
    Bios_param_block* const bpb = &fm->bpb;

    if (ff->super.type_dir == 0) {
        return NULL;
    }

    /* Traverse all chain FAT data cluster and read its structure. */
    Fat_file* ffiles = kmalloc(512 * sizeof(Fat_file));
    size_t file_cnt = 0;
    uint32_t limit = (bpb->sec_per_clus * bpb->bytes_per_sec) / sizeof(Dir_entry);
    uint32_t fe;
    uint32_t next_clus = ff->clus_num;
    uint32_t clus;
    do {
        clus = next_clus;
        if ((fm->fat_type == FAT_TYPE32) || ((uintptr_t)ff != (uintptr_t)fm->super.root_dir)) {
            /* ff is FAT32 or not root entry of FAT12/16.  */
            fe = fat_enrty_access(FILE_READ, fm, clus, 0);
            if (is_data_exist_fat_entry(fm, fe) == false) {
                /* Invalid */
                kfree(ffiles);
                return NULL;
            }

            /* FAT entry indicates last entry or next entry cluster. */
            next_clus = fe;

            /* Read entry. */
            if (fat_data_cluster_access(FILE_READ, fm, clus, fm->buffer) == NULL) {
                /* TODO: error handling. */
            }
        } else {
            /* ff is root entry of FAT12/16. */

            /* Set dummy entry data */
            switch (fm->fat_type) {
                case FAT_TYPE12:
                    fe = 0xFF8;
                    break;
                case FAT_TYPE16:
                    fe = 0xFFF8;
                    break;
            }

            /* read from root directory area. */
            fat_access(FILE_READ, fm, fm->rdentry.begin_sec, 1, fm->buffer);
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
            uint8_t checksum = calc_checksum(entry);
            bool is_error = false;

            /* Set name */
            char name[128];
            if (is_lfn(&de[i - 1]) == true) {
                /* There are LFN before SFN. */
                uint32_t j = i - 1;
                size_t char_num = 0;
                do {
                    if (checksum != lde[j].checksum) {
                        is_error = true;
                        break;
                    }
                    char_num += ucs2_to_ascii(lde[j].name0, name + char_num, 10);
                    char_num += ucs2_to_ascii(lde[j].name1, name + char_num, 12);
                    char_num += ucs2_to_ascii(lde[j].name2, name + char_num,  4);
                } while (j != 0 && (lde[j--].order & LFN_LAST_ENTRY_FLAG) == 0 && char_num < 102);
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
            f->clus_num         = (uint16_t)((entry->first_clus_num_hi << 16) | entry->first_clus_num_lo);
            f->super.type       = ((entry->attr & DIR_ATTR_DIRECTORY) != 0) ? FILE_TYPE_DIR : FILE_TYPE_FILE;
            f->super.size       = entry->file_size;
            f->super.belong_fs  = &fm->super;
            f->super.parent_dir = &ff->super;
            f->super.state_load = 0;

            file_cnt++;
        }
    } while (is_last_fat_entry(fm, fe) == false);

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

    return ff;
}


static inline Axel_state_code fat_access_file(uint8_t direction, File const* const f, uint8_t* const buffer) {
    if (f == NULL || buffer == NULL || f->type_file == 0 || f->size == 0) {
        return AXEL_FAILED;
    }

    Fat_file const * const ff = (Fat_file*)f;
    Fat_manager * const fm = (Fat_manager*)f->belong_fs;

    /* Allocate buffer that have enough size for reading/writing per cluster unit. */
    uint32_t const bpc = (fm->bpb.sec_per_clus * fm->bpb.bytes_per_sec);
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
        fe = fat_enrty_access(FILE_READ, fm, clus, 0);

        if (is_data_exist_fat_entry(fm, fe) == false) {
            /* Invalid. */
            goto failed;
        }

        /* FAT entry indicates last entry or next entry cluster. */
        next_clus = fe;

        /* Access data. */
        if (fat_data_cluster_access(rw, fm, clus, eb) != eb) {
            goto failed;
        }

        /* Shift Next cluster. */
        eb += bpc;
    } while (is_last_fat_entry(fm, fe) == false);

    if (rw == FILE_READ && eb < eb_limit) {
        /* error */
        goto failed;
    }

    if (rw == FILE_WRITE && eb < eb_limit) {
        /* Allocate new cluster because file size becomes bigger. */
        uint32_t alloc_clus;
        do {
            alloc_clus = alloc_cluster(fm);

            /* Set FAT chain. */
            if (alloc_clus == 0 || fat_enrty_access(FILE_WRITE, fm, clus, alloc_clus) == 0) {
                /* allocate failed */
                goto failed;
            }

            /* Write data. */
            if (fat_data_cluster_access(rw, fm, clus, eb) != eb) {
                goto failed;
            }

            /* Shift Next cluster. */
            eb += bpc;
        } while (eb < eb_limit);

        /* Set chain tail. */
        if (set_last_fat_entry(fm, clus) == 0) {
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
                    if (ft->current_dir->state_load == 0 && read_directory(ffi) == NULL) {
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

    /* Temporary buffer. */
    size_t const bytes_per_sec = dev->sector_size;
    uint8_t* const b = kmalloc(bytes_per_sec);

    Fat_manager* const fm = kmalloc_zeroed(sizeof(Fat_manager));
    fm->super.dev = dev;
    fm->super.pe = *pe;
    fm->super.change_dir = fat_change_dir;
    fm->super.access_file = fat_access_file;

    Fat_file* const root = kmalloc_zeroed(sizeof(Fat_file));
    root->super.name = kmalloc(1 + 1);
    root->super.belong_fs = &fm->super;
    root->super.type = FILE_TYPE_DIR | FILE_TYPE_ROOT;
    fm->super.current_dir = &root->super;
    fm->super.root_dir = &root->super;
    strcpy(root->super.name, "/");

    /* Load Bios parameter block. */
    if ((fat_access(FILE_READ, fm, 0, 1, b) != AXEL_SUCCESS) || (*((uint16_t*)(uintptr_t)(&b[510])) == 0x55aa)) {
        goto failed;
    }
    memcpy(&fm->bpb, b, sizeof(Bios_param_block));
    Bios_param_block* const bpb = &fm->bpb;

    /* Free temporary buffer and allocate buffer of one cluster. */
    kfree(b);
    fm->buffer_size = bpb->sec_per_clus * bytes_per_sec;
    fm->buffer = kmalloc(fm->buffer_size);

    /* These are logical sector number. */
    uint32_t fat_size     = (bpb->fat_size16 != 0) ? (bpb->fat_size16) : (bpb->fat32.fat_size32);
    uint32_t total_sec    = (bpb->total_sec16 != 0) ? (bpb->total_sec16) : (bpb->total_sec32);
    fm->rsvd.begin_sec    = 0;
    fm->rsvd.sec_nr       = bpb->rsvd_area_sec_num;
    fm->fat.begin_sec     = fm->rsvd.begin_sec + fm->rsvd.sec_nr;
    fm->fat.sec_nr        = fat_size * bpb->num_fats;
    fm->rdentry.begin_sec = fm->fat.begin_sec + fm->fat.sec_nr;;
    fm->rdentry.sec_nr    = (32 * bpb->root_ent_cnt + bpb->bytes_per_sec - 1) / bpb->bytes_per_sec;
    fm->data.begin_sec    = fm->rdentry.begin_sec + fm->rdentry.sec_nr;
    fm->data.sec_nr       = total_sec - fm->data.begin_sec;

    /* FAT type detection. */
    fm->cluster_nr = fm->data.sec_nr / bpb->sec_per_clus;
    if (fm->cluster_nr < 4085) {
        fm->fat_type = FAT_TYPE12;
        root->clus_num = 0;
    } else if (fm->cluster_nr < 65525) {
        fm->fat_type = FAT_TYPE16;
        root->clus_num = 0;
    } else {
        fm->fat_type = FAT_TYPE32;
        if (fsinfo_access(FILE_READ, fm) == NULL) {
            goto failed;
        }

        if (fm->fsinfo.free_cnt == FSINFO_INVALID_FREE) {
            /* TODO: count free cluster */
        }

        if (fm->fsinfo.next_free == FSINFO_INVALID_FREE) {
            /* TODO: set last allocated cluster number. */
        }

        root->clus_num = bpb->fat32.rde_clus_num;
    }

    /* Load root directory entry. */
    if (read_directory(root) == NULL) {
        goto failed;
    }

    return &fm->super;

failed:
    kfree(fm);
    kfree(fm->buffer);
    kfree(root);
    return NULL;
}
