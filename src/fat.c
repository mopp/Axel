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
            uint16_t fat32_version;    /* FAT32 volume version. */
            uint32_t root_clustor_num; /* First cluster number of root directory entry. */
            uint16_t fsinfo_sec_num;   /* Sector number of FSINFO struct location in reserved area(always 1). */
            uint16_t bk_boot_sec;      /* Backup of boot sector sector number. */
            uint8_t reserved[12];
            uint8_t drive_num; /* BIOS drive number. */
            uint8_t reserved1;
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



enum {
    FSINFO_LEAD_SIG         = 0x41615252,
    FSINFO_STRUCT_SIG       = 0x61417272,
    FSINFO_TRAIL_SIG        = 0xAA550000,
    FSINFO_INVALID_FREE     = 0xFFFFFFFF,
    DIR_STATE_FREE0         = 0x00,
    DIR_STATE_FREE1         = 0xe5,
    DIR_ATTR_READ_ONLY      = 0x01,
    DIR_ATTR_HIDDEN         = 0x02,
    DIR_ATTR_SYSTEM         = 0x04,
    DIR_ATTR_VOLUME_ID      = 0x08,
    DIR_ATTR_DIRECTORY      = 0x10,
    DIR_ATTR_ARCHIVE        = 0x20,
    DIR_ATTR_LONG_NAME      = DIR_ATTR_READ_ONLY | DIR_ATTR_HIDDEN | DIR_ATTR_SYSTEM | DIR_ATTR_VOLUME_ID,
    DIR_ATTR_LONG_NAME_MASK = DIR_ATTR_READ_ONLY | DIR_ATTR_HIDDEN | DIR_ATTR_SYSTEM | DIR_ATTR_VOLUME_ID | DIR_ATTR_DIRECTORY | DIR_ATTR_ARCHIVE,
    FAT_TYPE12              = 12,
    FAT_TYPE16              = 16,
    FAT_TYPE32              = 32,
    FAT_ENTRY_WRITE,
    FAT_ENTRY_READ,
};


struct fat_manager {
    File_system fs;
    Bios_param_block bpb;
    Fsinfo fsinfo;
    uint8_t* buffer;
    size_t buffer_size;
    uint8_t fat_type;
};
typedef struct fat_manager Fat_manager;


Axel_state_code fat_access(uint8_t direction, Fat_manager const* ft, uint32_t lba, uint8_t sec_cnt, uint8_t* buf) {
    return ata_access(direction, ft->fs.dev, ft->fs.pe.lba_first + lba, sec_cnt, buf);
}


uint32_t fat_enrty_access(uint8_t direction, Fat_manager* ft, uint32_t n, uint32_t write_entry) {
    uint32_t offset       = n * ((ft->fat_type == FAT_TYPE32) ? 4 : 2);
    uint32_t bps          = ft->bpb.bytes_per_sec;
    uint32_t sec_num      = ft->bpb.rsvd_area_sec_num + (offset / bps);
    uint32_t entry_offset = offset % bps;

    if (fat_access(ATA_READ, ft, sec_num, 1, ft->buffer) == AXEL_FAILED) {
        return 0;
    }
    uint32_t* b = (uint32_t*)(uintptr_t)&ft->buffer[entry_offset];
    uint32_t read_entry = *b & 0x0FFFFFFF;

    if (direction == FAT_ENTRY_READ) {
        return read_entry;
    }

    /* change FAT entry. */
    *b = (read_entry & 0xF0000000) | (write_entry & 0x0FFFFFFF);

    if (fat_access(ATA_WRITE, ft, sec_num, 1, ft->buffer) == AXEL_FAILED) {
        return 0;
    }

    return *b;
}


uint8_t* ucs2_to_ascii(uint8_t* ucs2, uint8_t* ascii, size_t ucs2_len) {
    if ((ucs2_len & 0x1) == 1) {
        /* invalid. */
        return NULL;
    }

    size_t cnt = 0;
    for(size_t i = 0; i < ucs2_len; i++) {
        uint8_t c = ucs2[i];
        if((0 <= c) && (c < 127)) {
            ascii[cnt++] = ucs2[i];
        }
    }
    ascii[cnt] = '\0';

    return ascii;
}


/* LBA = (H + C * Total Head size) * (Sector size per track) + (S - 1) */
File_system* init_fat(Ata_dev* dev, Partition_entry* pe) {
    if (dev == NULL || pe == NULL) {
        return NULL;
    }

    Fat_manager* ft = kmalloc(sizeof(Fat_manager));
    ft->fs.dev      = dev;
    ft->fs.pe       = *pe;
    ft->buffer_size = dev->sector_size * 4;
    ft->buffer      = kmalloc(ft->buffer_size);

    /* Load Bios parameter block. */
    if (fat_access(ATA_READ, ft, 0, 1, ft->buffer) != AXEL_SUCCESS) {
        kfree(ft);
        return NULL;
    }

    if (*((uint16_t*)(uintptr_t)(&ft->buffer[510])) == 0x55aa) {
        /* Invalid BPB. */
        kfree(ft);
        return NULL;
    }
    memcpy(&ft->bpb, ft->buffer, sizeof(Bios_param_block));
    /* FIXME: bpb->sec_per_clus; */

    Bios_param_block* bpb = &ft->bpb;
    printf("%s\n", bpb->oem_name);
    printf("Boot signature      : 0x%x\n", bpb->fat32.boot_sig);
    printf("Total size          : %d MB\n", MB(bpb->total_sec32 * bpb->bytes_per_sec));

    /* These are logical sector number. */
    uint32_t fat_size           = (bpb->fat_size16 != 0) ? (bpb->fat_size16) : (bpb->fat32.fat_size32);
    uint32_t total_sec          = (bpb->total_sec16 != 0) ? (bpb->total_sec16) : (bpb->total_sec32);
    uint32_t rsvd_first_sec     = 0;
    uint32_t rsvd_sec_num       = bpb->rsvd_area_sec_num;
    uint32_t fat_first_sec      = rsvd_sec_num;
    uint32_t fat_sec_size       = fat_size * bpb->num_fats;
    uint32_t root_dir_first_sec = fat_first_sec + fat_sec_size;
    uint32_t root_dir_sec_num   = (32 * bpb->root_ent_cnt + bpb->bytes_per_sec - 1) / bpb->bytes_per_sec;
    uint32_t data_first_sec     = root_dir_first_sec + root_dir_sec_num;
    uint32_t data_sec_num       = total_sec - data_first_sec;

    printf("base sector         : %d\n", pe->lba_first);
    printf("Area, first sector, sector num\n");
    printf("rsvd, %12d, %10d\n", rsvd_first_sec, rsvd_sec_num);
    printf("FAT , %12d, %10d\n", fat_first_sec, fat_sec_size);
    printf("Root, %12d, %10d\n", root_dir_first_sec, root_dir_sec_num);
    printf("Data, %12d, %10d\n", data_first_sec, data_sec_num);
    printf("sector per cluster: %d\n", bpb->sec_per_clus);

    /* FAT type detection. */
    uint32_t cluster_num = data_sec_num / bpb->sec_per_clus;
    uint8_t fat_type;
    if (cluster_num < 4085) {
        fat_type = FAT_TYPE12;
    } else if (cluster_num < 65525) {
        fat_type = FAT_TYPE16;
    } else {
        fat_type = FAT_TYPE32;
    }
    ft->fat_type = fat_type;

    /*
     * NOTE: that the CountofClusters value is exactly that the count of data clusters starting at cluster 2.
     * The maximum valid cluster number for the volume is CountofClusters + 1, and the “count of clusters
     * including the two reserved clusters” is CountofClusters + 2.
     */
    printf("Cluster num : %d -> FAT%d\n", cluster_num, fat_type);

    /* Load FSINFO struct. */
    if (fat_access(ATA_READ, ft, bpb->fat32.fsinfo_sec_num, 1, ft->buffer) == AXEL_FAILED) {
        kfree(ft);
        return NULL;
    }
    memcpy(&ft->fsinfo, ft->buffer, sizeof(Fsinfo));

    Fsinfo* fsinfo = &ft->fsinfo;
    if ((fsinfo->lead_signature != FSINFO_LEAD_SIG) || (fsinfo->struct_signature != FSINFO_STRUCT_SIG) || (fsinfo->trail_signature != FSINFO_TRAIL_SIG)) {
        /* invalid. */
        kfree(ft);
        return NULL;
    }
    printf("%d\n", fsinfo->free_cnt);

    printf("FAT num     : %d\n", bpb->num_fats);
    printf("FAT sec num : %d\n", fat_size);
    if (fat_type == FAT_TYPE32) {
        printf("dir entry cluster num : %d\n", bpb->fat32.root_clustor_num);
        printf("0x%x\n", fat_enrty_access(FAT_ENTRY_READ, ft, bpb->fat32.root_clustor_num, 0));
        fat_access(ATA_READ, ft, data_first_sec + bpb->fat32.root_clustor_num - 2, bpb->sec_per_clus, ft->buffer);
        Dir_entry* de = (Dir_entry*)(uintptr_t)ft->buffer;
        Long_dir_entry* lde = (Long_dir_entry*)(uintptr_t)ft->buffer;
        char name[11];
        printf("0x%x  ", de[0].attr & DIR_ATTR_LONG_NAME_MASK);
        printf("0x%x\n", de[1].attr & DIR_ATTR_LONG_NAME_MASK);
        printf("%s", ucs2_to_ascii(lde[0].name0, name, 10));
        printf("%s\n", ucs2_to_ascii(lde[1].name0, name, 10));
        printf("%s", ucs2_to_ascii(lde[2].name0, name, 10));
        printf("%s\n", ucs2_to_ascii(lde[3].name0, name, 10));
    }


    return &ft->fs;
}


uint8_t calc_checksum(Dir_entry const* de) {
    int i;
    uint8_t sum;

    for (i = sum = 0; i < 11; i++) {
        sum = (sum >> 1u) + (uint8_t)(sum << 7u) + de->name[i];
    }

    return sum;
}
