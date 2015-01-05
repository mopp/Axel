/*
 *                     Disk Fat_image maps
 *    0 byte +---------------+----------------+
 *           |       Master Boot Record       |
 *           |       (First boot loader)      |
 *  512 byte +--------------------------------+
 *           |                                |
 *           ~       Second Boot Loader       ~
 *           |                                |
 *           +--------------------------------+
 *           |                                |
 *           |                                |
 *           ~        First Partition         ~
 *           |                                |
 *           |                                |
 *           +--------------------------------+
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include "../include/fat.h"
#include "../include/fat_manip.h"
#include "../include/macros.h"


#define error_msg(fmt, ...) fprintf(stderr, "ERROR " fmt, __VA_ARGS__)
#define error_echo(fmt) fprintf(stderr, "ERROR " fmt)


enum {
    SECTOR_SIZE = 512,
};


typedef struct {
    uint8_t* img_buffer;
    size_t img_size;
    Master_boot_record* mbr;
    char const* img_path;
    char const* mbr_path;
    char const* loader2_path;
    unsigned char active_partition;
    Fat_manips manip;
} Fat_image;


static int set_options(Fat_image*, int, char* const[]);
static int create_image(Fat_image*);
static int load_loader(Fat_image*);
static int construct_fat(Fat_image*);
static Axel_state_code block_access(void*, uint8_t, uint32_t, uint8_t, uint8_t*);
static long get_file_size(char const*);
static void* load_file(char const*, void*, size_t);
static size_t lba_to_byte(size_t);


int main(int argc, char* const argv[]) {
    Fat_image img;
    if (set_options(&img, argc, argv) != 0) {
        return EXIT_FAILURE;
    }

    if (create_image(&img) == 0) {
        puts("Create image succeed");
        return 0;
    } else {
        puts("Create image failed");
        return EXIT_FAILURE;
    }
}


static inline int set_options(Fat_image* img, int argc, char* const argv[]) {
    img->img_path = NULL;
    img->mbr_path = NULL;
    img->loader2_path = NULL;
    img->img_size = 0;
    img->manip.fat_type = FAT_TYPE32;

    while (1) {
        int opt = getopt(argc, argv, "hvs:");

        if (opt == -1) {
            /* getopt() returns -1 when all command line options are parsed. */
            break;
        }

        int t;
        switch (opt) {
            case 's':
                t = atoi(optarg);
                if (t <= 0) {
                    error_msg("Size is invalid: %d", t);
                    return EXIT_FAILURE;
                }
                img->img_size = (size_t)t;
                break;
            case 'f':
                t = atoi(optarg);
                if ((t != 12) && (t != 16) && (t != 32)) {
                    error_msg("Invalid FAT type: %d", t);
                    return EXIT_FAILURE;
                }
                img->manip.fat_type = (unsigned char)t;
                break;
            case 'v':
            case 'h':
                puts(
                    "Fat_image util for Axel.\n"
                    "Usage: aim [-s IMAGE_SIZE MB] TARGET_FILE_NAME MBR FILE\n"
                    "  default values\n"
                    "    IMAGE_SIZE: 32 MB");
                return EXIT_FAILURE;
        }
    }

    /* Deal with non-option arguments here */
    for (size_t i = 0; optind < argc; i++, optind++) {
        char const* t = argv[optind];
        switch (i) {
            case 0:
                img->img_path = t;
                break;
            case 1:
                img->mbr_path = t;
                break;
            case 2:
                img->loader2_path = t;
                break;
        }
    }

    if (img->img_size == 0) {
        img->img_size = 33 * 1024 * 1024;
    }
    img->img_size = ALIGN_UP(img->img_size, SECTOR_SIZE);

    if (img->img_path == NULL) {
        error_echo("Please input target file path\n");
        return EXIT_FAILURE;
    }

    if (img->mbr_path == NULL) {
        error_echo("Please input MBR file path\n");
        return EXIT_FAILURE;
    }

    if (img->loader2_path == NULL) {
        error_echo("Please input second loader file path\n");
        return EXIT_FAILURE;
    }

    return 0;
}


static inline int create_image(Fat_image* img) {
    img->img_buffer = malloc(img->img_size * sizeof(char));
    if (img->img_buffer == NULL) {
        error_msg("Alloc failed: size %zu", img->img_size);
        goto failed;
    }
    memset(img->img_buffer, 0, img->img_size * sizeof(char));

    if (load_loader(img) != 0) {
        error_echo("Loading boot loader failed\n");
        goto failed;
    }

    Fsinfo mem_fsinfo;
    memset(&mem_fsinfo, 0, sizeof(Fsinfo));
    img->manip.fsinfo = &mem_fsinfo;
    if (construct_fat(img) != 0) {
        error_echo("Setting FAT failed\n");
        goto failed;
    }

    FILE* fp = fopen(img->img_path, "w+b");
    if (fp == NULL) {
    }
    size_t r = fwrite(img->img_buffer, sizeof(char), img->img_size, fp);
    fclose(fp);

    free(img->img_buffer);

    return (r < img->img_size) ? EXIT_FAILURE : 0;

failed:
    free(img->img_buffer);
    return EXIT_FAILURE;
}


static inline int load_loader(Fat_image* img) {
    uint8_t* const buf = img->img_buffer;

    /* Check size */
    long tmp = get_file_size(img->mbr_path);
    if (tmp <= 0) {
        error_echo("Invalid MBR");
        return EXIT_FAILURE;
    }

    if (tmp != SECTOR_SIZE) {
        error_msg("Sector size(%d) not equals MBR size(%lu)", SECTOR_SIZE, tmp);
        return EXIT_FAILURE;
    }

    /* Load MBR */
    void* r = load_file(img->mbr_path, buf, SECTOR_SIZE);
    if (r == NULL) {
        return EXIT_FAILURE;
    }

    /* This indicates tail of wrote area in buf. */
    size_t img_buf_idx = SECTOR_SIZE;

    /* Check valid MBR */
    Master_boot_record* mbr = (Master_boot_record*)buf;
    img->mbr = mbr;
    if (mbr->boot_signature != BOOT_SIGNATURE) {
        error_echo("MBR is invalid.");
        return EXIT_FAILURE;
    }

    /* Load second loader */
    tmp = get_file_size(img->loader2_path);
    if (tmp <= 0) {
        error_echo("Second loader size is invalid");
        return EXIT_FAILURE;
    }
    size_t second_loader_size = (size_t)tmp;
    r = load_file(img->loader2_path, buf + img_buf_idx, second_loader_size);
    if (r == NULL) {
        return EXIT_FAILURE;
    }

    /* Roundup */
    second_loader_size = ALIGN_UP(second_loader_size, SECTOR_SIZE);

    /*
     * Set MBR config to make first partition FAT32.
     * Allocate second loader before first Partition
     */
    img->active_partition = 0;
    mbr->p_entry[img->active_partition].bootable = 0x80;
    mbr->p_entry[img->active_partition].type = PART_TYPE_FAT32_LBA;
    mbr->p_entry[img->active_partition].lba_first = (uint32_t)((SECTOR_SIZE + second_loader_size) / SECTOR_SIZE);
    mbr->p_entry[img->active_partition].sector_nr = (uint32_t)((img->img_size / SECTOR_SIZE) - mbr->p_entry[0].lba_first);

    return 0;
}


static inline int construct_fat(Fat_image* img) {
    Fat_manips* fm = &img->manip;
    Master_boot_record* mbr = img->mbr;
    Bios_param_block* bpb = (Bios_param_block*)((uintptr_t)img->img_buffer + (lba_to_byte(mbr->p_entry[img->active_partition].lba_first)));

    /* Set manipulator */
    fm->bpb = bpb;
    fm->b_access = block_access;
    fm->alloc = malloc;
    fm->free = free;
    fm->obj = img;

    /* Setting BIOS Parameter Block */
    memcpy(bpb->oem_name, "AXELMOPP", 8);
    bpb->bytes_per_sec = SECTOR_SIZE;
    bpb->sec_per_clus = 1;
    bpb->num_fats = 2;
    bpb->media = 0xf8;

    if (img->manip.fat_type == FAT_TYPE32) {
        bpb->root_ent_cnt = 0;
        bpb->total_sec16 = 0;
        bpb->fat_size16 = 0;
        bpb->rsvd_area_sec_num = 4;
        bpb->total_sec32 = mbr->p_entry[img->active_partition].sector_nr;

        /* Calculate the number of cluster. */
        size_t spc = bpb->sec_per_clus;
        size_t sectors = bpb->total_sec32 - bpb->rsvd_area_sec_num;
        size_t fat_entry_nr = (sectors / spc);
        size_t all_fat_entry_size = ALIGN_UP(fat_entry_nr * 4u, SECTOR_SIZE);

        /* Assume FAT area size. */
        size_t assumed_fat_sectors = (all_fat_entry_size * bpb->num_fats) / SECTOR_SIZE;
        fat_entry_nr = (sectors - assumed_fat_sectors) / spc;
        all_fat_entry_size = ALIGN_UP(fat_entry_nr * 4u, SECTOR_SIZE);

        /* Calculate actual the number of sector per a FAT. */
        bpb->fat32.fat_size32 = (uint16_t)(all_fat_entry_size / SECTOR_SIZE);
        bpb->fat32.rde_clus_num = 2;
        bpb->fat32.fsinfo_sec_num = 1;

        fat_calc_sectors(bpb, &fm->area);

        printf("fat size: %d\n", bpb->fat32.fat_size32);
        printf("fat entries %d\n", bpb->fat32.fat_size32 * SECTOR_SIZE / 4);
        printf("Total cluster %d\n", bpb->total_sec32);
        printf("rsvd begin   : %5d\n", fm->area.rsvd.begin_sec);
        printf("rsvd    nr   : %5d\n", fm->area.rsvd.sec_nr);
        printf("fat  begin   : %5d\n", fm->area.fat.begin_sec);
        printf("fat     nr   : %5d\n", fm->area.fat.sec_nr);
        printf("rdentry begin: %5d\n", fm->area.rdentry.begin_sec);
        printf("rdentry    nr: %5d\n", fm->area.rdentry.sec_nr);
        printf("data begin   : %5d\n", fm->area.data.begin_sec);
        printf("data    nr   : %5d\n", fm->area.data.sec_nr);

        /* Init FAT[0] and FAT[1]. */
        uint32_t fat_entry = 0xFFFFFF00 | bpb->media;
        fat_entry_access(fm, FILE_WRITE, 0, fat_entry);
        fat_entry = 0xFFFFFFFF;
        fat_entry_access(fm, FILE_WRITE, 1, fat_entry);

        /* Init FSINFO structure. */
        fm->fsinfo->lead_signature = FSINFO_LEAD_SIG;
        fm->fsinfo->struct_signature = FSINFO_STRUCT_SIG;
        fm->fsinfo->free_cnt = (uint32_t)(fm->area.data.sec_nr / bpb->sec_per_clus) - 1; /* -1 for root directory entry. */
        fm->fsinfo->next_free = 2;                                                       /* FAT[0], FAT[1] are reserved FAT entry, Thus start point is FAT[2]. */
        fm->fsinfo->trail_signature = FSINFO_TRAIL_SIG;
        fat_fsinfo_access(fm, FILE_WRITE, fm->fsinfo);
        printf("fsinfo free_cnt: %d\n", fm->fsinfo->free_cnt);

        /* Init root directory */
        uint32_t rood_dir_cluster_num = bpb->fat32.rde_clus_num;
        set_last_fat_entry(fm, rood_dir_cluster_num);
        printf("Root dir entry cluster: %d\n", rood_dir_cluster_num);
        int r = fat_make_directory(fm, rood_dir_cluster_num, "bin", DIR_ATTR_READ_ONLY);
        if (r != AXEL_SUCCESS) {
            printf("make dir failed\n");
        } else {
            printf("make dir success\n");
        }
    } else if (img->manip.fat_type == FAT_TYPE16) {
        bpb->rsvd_area_sec_num = 1;
        /* TODO: */
    } else {
        bpb->rsvd_area_sec_num = 1;
        /* TODO: */
    }

    return 0;
}


static inline Axel_state_code block_access(void* p, uint8_t direction, uint32_t lba, uint8_t sec_cnt, uint8_t* buf) {
    Fat_image* ft = p;

    size_t base_lba = ft->mbr->p_entry[ft->active_partition].lba_first;
    size_t offset = lba_to_byte(base_lba + lba);
    size_t size = lba_to_byte(sec_cnt);
    void* target = &ft->img_buffer[offset];

    if (direction == FILE_READ) {
        memcpy(buf, target, size);
    } else if (direction == FILE_WRITE) {
        memcpy(target, buf, size);
    } else {
        error_echo("unknown direction in block_access");
        return AXEL_FAILED;
    }

    return AXEL_SUCCESS;
}


static inline long get_file_size(char const* fname) {
    FILE* fp = fopen(fname, "rb");
    if (fp == NULL) {
        return 0;
    }

    fseek(fp, 0, SEEK_END);
    long s = ftell(fp);

    fclose(fp);

    return s;
}


static inline void* load_file(char const* fname, void* buf, size_t read_size) {
    FILE* fmbr = fopen(fname, "r+b");
    if (fmbr == NULL) {
        return NULL;
    }

    size_t r = fread(buf, 1, read_size, fmbr);
    fclose(fmbr);

    if (r <= 0) {
        error_msg("fread failed: %s\n", fname);
        return NULL;
    }

    return buf;
}


static inline size_t lba_to_byte(size_t nr) {
    return nr * SECTOR_SIZE;
}
