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
            uint16_t fat_size32; /* the number of sector of one FAT. */
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
            uint8_t boot_sig;         /* extend boot signature. */
            uint32_t volume_id;       /* volume serial number. */
            uint8_t volume_label[11]; /* partition volume label */
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


struct fat_manager {
    File_system fs;
    Bios_param_block bpb;
    Fsinfo fsinfo;
    uint8_t* buffer;
    size_t buffer_size;
    uint32_t data_area_first_sec;
    uint32_t data_area_sec_nr;
    uint8_t fat_type;
};
typedef struct fat_manager Fat_manager;


enum {
    FSINFO_LEAD_SIG         = 0x41615252,
    FSINFO_STRUCT_SIG       = 0x61417272,
    FSINFO_TRAIL_SIG        = 0xAA550000,
    FSINFO_INVALID_FREE     = 0xFFFFFFFF,
    DIR_LAST_FREE     = 0x00,
    DIR_FREE          = 0xe5,
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
    FAT_WRITE               = ATA_WRITE,
    FAT_READ                = ATA_READ,
};


static inline Axel_state_code fat_access(uint8_t direction, Fat_manager const* ft, uint32_t lba, uint8_t sec_cnt, uint8_t* buf) {
    return ata_access(direction, ft->fs.dev, ft->fs.pe.lba_first + lba, sec_cnt, buf);
}


static inline uint32_t fat_enrty_access(uint8_t direction, Fat_manager const* ft, uint32_t n, uint32_t write_entry) {
    uint32_t offset       = n * ((ft->fat_type == FAT_TYPE32) ? 4 : 2);
    uint32_t bps          = ft->bpb.bytes_per_sec;
    uint32_t sec_num      = ft->bpb.rsvd_area_sec_num + (offset / bps);
    uint32_t entry_offset = offset % bps;

    if (fat_access(FAT_READ, ft, sec_num, 1, ft->buffer) == AXEL_FAILED) {
        return 0;
    }
    uint32_t* b = (uint32_t*)(uintptr_t)&ft->buffer[entry_offset];
    uint32_t read_entry = *b & 0x0FFFFFFF;

    if (direction == FAT_READ) {
        return read_entry;
    }

    /* Change FAT entry. */
    *b = (read_entry & 0xF0000000) | (write_entry & 0x0FFFFFFF);

    if (fat_access(FAT_WRITE, ft, sec_num, 1, ft->buffer) == AXEL_FAILED) {
        return 0;
    }

    return *b;
}


static inline uint8_t* fat_data_cluster_access(uint8_t direction, Fat_manager const * ft, uint32_t clus_num, uint8_t* buffer) {
    uint8_t spc = ft->bpb.sec_per_clus;
    uint32_t sec = ft->data_area_first_sec + (clus_num - 2) * spc;
    if (fat_access(FAT_READ, ft, sec, spc, buffer) == AXEL_FAILED) {
        return NULL;
    }

    return ft->buffer;
}


static inline bool is_unused_fat_entry(uint32_t fe) {
    return (fe == 0) ? true : false;
}


static inline bool is_rsvd_fat_entry(uint32_t fe) {
    return (fe == 1) ? true : false;
}


static inline bool is_bad_clus_fat_entry(uint32_t fe) {
    return (fe == 0x0FFFFFF7) ? true : false;
}


static inline bool is_valid_data_fat_entry(uint32_t fe) {
    /* NOT bad cluster and reserved. */
    return (is_bad_clus_fat_entry(fe) == false && is_rsvd_fat_entry(fe) == false) ? true : false;
}


static inline bool is_data_exist_fat_entry(uint32_t fe) {
    return (is_unused_fat_entry(fe) == false && is_valid_data_fat_entry(fe) == true) ? true : false;
}


static inline bool is_last_fat_entry(Fat_manager const* ft, uint32_t fe) {
    return (0x0FFFFFF8 <= fe && fe <= 0x0FFFFFFF) ? true : false;
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
    Fat_file* ffiles = kmalloc(128 * sizeof(Fat_file));
    size_t file_cnt = 0;
    uint32_t limit = (bpb->sec_per_clus * bpb->bytes_per_sec) / sizeof(Dir_entry);
    uint32_t fe;
    uint32_t next_clus = ff->clus_num;
    uint32_t clus;
    do {
        clus = next_clus;
        fe = fat_enrty_access(FAT_READ, fm, clus, 0);
        if (is_data_exist_fat_entry(fe) == false) {
            /* Invalid */
            kfree(ffiles);
            return NULL;
        }

        /* FAT entry indicates last entry or next entry cluster. */
        next_clus = fe;

        /* Read data. */
        fat_data_cluster_access(FAT_READ, fm, clus, fm->buffer);
        Dir_entry* const de       = (Dir_entry*)(uintptr_t)fm->buffer;
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
            Fat_file* f = &ffiles[file_cnt];
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
                } while (j != 0 && (lde[j--].order & LFN_LAST_ENTRY_FLAG) == 0);
            } else {
                /* SFN stand alone. */
                memcpy(name, entry->name, 11);
                name[11] = '\0';
            }
            if (is_error == true) {
                is_error = false;
                continue;
            }

            trim_tail(name);
            size_t len = strlen(name);
            f->super.name = kmalloc_zeroed(sizeof(uint8_t) * (len + 1));
            strcpy(f->super.name, name);

            /* Set the others. */
            f->clus_num         = (uint16_t)((entry->first_clus_num_hi << 16) | entry->first_clus_num_lo);
            f->super.type       = ((entry->attr & DIR_ATTR_DIRECTORY) != 0) ? FILE_TYPE_DIR : FILE_TYPE_FILE;
            f->super.size       = entry->file_size;
            f->super.belong_fs  = &fm->fs;
            f->super.parent_dir = &ff->super;
            f->super.state_load = 0;

            file_cnt++;
        }
    } while (is_last_fat_entry(fm, fe) == false);

    /* FIXME: copy and alloc each area. */
    /* Copy into memory area which has appropriate size. */
    Fat_file* exist_ffiles = kmalloc_zeroed(sizeof(Fat_file) * file_cnt);
    memcpy(exist_ffiles, ffiles, sizeof(Fat_file) * file_cnt);
    kfree(ffiles);

    /* Store file infomation */
    ff->super.children = kmalloc_zeroed(sizeof(File*) * file_cnt);
    ff->super.child_nr = file_cnt;
    ff->super.state_load = 1;
    for (size_t i = 0; i < file_cnt; i++) {
        ff->super.children[i] = &exist_ffiles[i].super;
    }

    return ff;
}


static inline Axel_state_code fat_access_file(uint8_t direction, File const* f, uint8_t* buffer) {
    if (f == NULL || buffer == NULL || f->type_file == 0) {
        return AXEL_FAILED;
    }

    Fat_file const * const ff = (Fat_file*)f;
    Fat_manager const* const fm = (Fat_manager*)f->belong_fs;
    uint32_t fe, clus, next_clus;
    next_clus = ff->clus_num;

    /* Allocate buffer that have enough size for reading/writing per cluster unit. */
    uint32_t const bpc = (fm->bpb.sec_per_clus * fm->bpb.bytes_per_sec);
    uint32_t const rounded_size = (uint32_t)((f->size + bpc - 1) / bpc) * bpc;
    uint8_t* const ebuffer = kmalloc(rounded_size);
    uint8_t* eb = ebuffer ;

    uint8_t rw;
    if (direction == FILE_WRITE) {
        rw = FAT_WRITE;
        memcpy(eb, buffer, f->size);
    } else {
        rw = FAT_READ;
    }

    do {
        clus = next_clus;
        fe = fat_enrty_access(FAT_READ, fm, clus, 0);

        if (is_data_exist_fat_entry(fe) == false) {
            /* Invalid. */
            kfree(ebuffer);
            return AXEL_FAILED;
        }

        /* FAT entry indicates last entry or next entry cluster. */
        next_clus = fe;

        /* Read data. */
        if (fat_data_cluster_access(rw, fm, clus, eb) == NULL) {
            kfree(ebuffer);
            return AXEL_FAILED;
        }

        /* Shift Next cluster. */
        eb += bpc;
    } while (is_last_fat_entry(fm, fe) == false);

    if (direction == FILE_READ) {
        memcpy(buffer, ebuffer, f->size);
    }

    kfree(ebuffer);

    return AXEL_SUCCESS;
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

    Fat_manager* const fman = kmalloc(sizeof(Fat_manager));
    fman->fs.dev = dev;
    fman->fs.pe = *pe;
    fman->fs.change_dir = fat_change_dir;
    fman->fs.access_file = fat_access_file;

    Fat_file* const root  = kmalloc_zeroed(sizeof(Fat_file));
    root->super.name      = kmalloc(1 + 1);
    root->super.belong_fs = &fman->fs;
    root->super.type      = FILE_TYPE_DIR | FILE_TYPE_ROOT;
    fman->fs.current_dir  = &root->super;
    fman->fs.root_dir     = &root->super;
    strcpy(root->super.name, "/");

    /* Load Bios parameter block. */
    if ((fat_access(FAT_READ, fman, 0, 1, b) != AXEL_SUCCESS) || (*((uint16_t*)(uintptr_t)(&b[510])) == 0x55aa)) {
        goto failed;
    }
    memcpy(&fman->bpb, b, sizeof(Bios_param_block));
    Bios_param_block* const bpb = &fman->bpb;

    /* Free temporary buffer and allocate buffer of one cluster. */
    kfree(b);
    fman->buffer = kmalloc(bpb->sec_per_clus * bytes_per_sec);

    /* These are logical sector number. */
    uint32_t fat_size           = (bpb->fat_size16 != 0) ? (bpb->fat_size16) : (bpb->fat32.fat_size32);
    uint32_t total_sec          = (bpb->total_sec16 != 0) ? (bpb->total_sec16) : (bpb->total_sec32);
    /* uint32_t rsvd_first_sec     = 0; */
    uint32_t rsvd_sec_num       = bpb->rsvd_area_sec_num;
    uint32_t fat_first_sec      = rsvd_sec_num;
    uint32_t fat_sec_size       = fat_size * bpb->num_fats;
    uint32_t root_dir_first_sec = fat_first_sec + fat_sec_size;
    uint32_t root_dir_sec_num   = (32 * bpb->root_ent_cnt + bpb->bytes_per_sec - 1) / bpb->bytes_per_sec;
    uint32_t data_first_sec     = root_dir_first_sec + root_dir_sec_num;
    uint32_t data_sec_num       = total_sec - data_first_sec;
    fman->data_area_first_sec     = data_first_sec;
    fman->data_area_sec_nr        = data_sec_num;

    /* FAT type detection. */
    uint32_t cluster_num = data_sec_num / bpb->sec_per_clus;
    if (cluster_num < 4085) {
        fman->fat_type = FAT_TYPE12;
    } else if (cluster_num < 65525) {
        fman->fat_type = FAT_TYPE16;
    } else {
        fman->fat_type = FAT_TYPE32;
        /* Load FSINFO struct. */
        if (fat_access(FAT_READ, fman, bpb->fat32.fsinfo_sec_num, 1, fman->buffer) == AXEL_FAILED) {
            goto failed;
        }
        memcpy(&fman->fsinfo, fman->buffer, sizeof(Fsinfo));

        Fsinfo* fsinfo = &fman->fsinfo;
        if ((fsinfo->lead_signature != FSINFO_LEAD_SIG) || (fsinfo->struct_signature != FSINFO_STRUCT_SIG) || (fsinfo->trail_signature != FSINFO_TRAIL_SIG)) {
            /* invalid. */
            goto failed;
        }

        /* Load root directory entry. */
        root->clus_num = bpb->fat32.rde_clus_num;
        if (read_directory(root) == NULL) {
            goto failed;
        }
    }

    return &fman->fs;

failed:
    kfree(fman);
    kfree(fman->buffer);
    kfree(root);
    return NULL;
}
