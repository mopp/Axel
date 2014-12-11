#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include "../include/fat.h"
#include "../include/macros.h"


#define error_msg(fmt, ...) fprintf(stderr, "ERROR " fmt, __VA_ARGS__)
#define error_echo(fmt) fprintf(stderr, "ERROR " fmt)


enum {
    SECTOR_SIZE = 512,
};

typedef struct {
    void* buf;
    char const* name;
    char const* mbr_path;
    char const* second_loader_path;
    size_t size;
    unsigned int active_partition;
    unsigned int fat_type;
} Image;



static int set_options(Image*, int, char* const[]);
static int create_image(Image*);


int main(int argc, char* const argv[]) {
    Image img = {.name = NULL, .mbr_path = NULL, .size = 0, .fat_type = 32};

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


static inline void* load_file(char const* fname, void* buf, size_t size) {
    FILE* fmbr = fopen(fname, "r+b");
    if (fmbr == NULL) {
        return NULL;
    }

    size_t r = fread(buf, 1, size, fmbr);
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


/*
 *                     Disk Image maps
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
static inline int load_loader(Image* img) {
    char* const buf = img->buf;

    /* Check size */
    long t = get_file_size(img->mbr_path);
    if (t <= 0) {
        error_echo("Invalid MBR");
        return EXIT_FAILURE;
    }

    if (t != SECTOR_SIZE) {
        error_msg("Sector size(%d) and MBR size(%lu) miss match", SECTOR_SIZE, t);
        return EXIT_FAILURE;
    }

    /* Load MBR */
    void* r = load_file(img->mbr_path, buf, SECTOR_SIZE);
    if (r == NULL) {
        return EXIT_FAILURE;
    }

    /* This indicates tail of used area in buf. */
    size_t write_size = SECTOR_SIZE;

    /* Check valid MBR */
    Master_boot_record* mbr = (Master_boot_record*)buf;
    if (mbr->boot_signature != BOOT_SIGNATURE) {
        error_echo("MBR is invalid.");
        return EXIT_FAILURE;
    }

    /* Load second loader */
    t = get_file_size(img->second_loader_path);
    if (t <= 0) {
        error_echo("Second loader size is invalid");
        return EXIT_FAILURE;
    }
    size_t secl_size = (size_t)t;
    r = load_file(img->second_loader_path, buf + write_size, secl_size);
    if (r == NULL) {
        return EXIT_FAILURE;
    }

    /* Roundup */
    secl_size = ALIGN_UP(secl_size, SECTOR_SIZE);

    /*
     * Set MBR config to make first partition FAT32.
     * Allocate 8 MB before first Partition
     */
    mbr->p_entry[0].bootable = 0x80;
    mbr->p_entry[0].type = PART_TYPE_FAT32_LBA;
    mbr->p_entry[0].lba_first = (SECTOR_SIZE + secl_size) / SECTOR_SIZE;
    mbr->p_entry[0].sector_nr = (img->size / SECTOR_SIZE) - mbr->p_entry[0].lba_first;
    img->active_partition = 0;

    return 0;
}


static inline void* access_sector(Image* img, size_t sec_nr, void* buf) {
    return memcpy((void*)((uintptr_t)img->buf + lba_to_byte(sec_nr)), buf, SECTOR_SIZE);
}


static inline void* access_partition(Image* img, size_t sec_nr, void* buf) {
    Master_boot_record* mbr = img->buf;
    return access_sector(img,mbr->p_entry[img->active_partition].lba_first + sec_nr, buf);
}


static inline int set_fat(Image* img) {
    char* const buf = img->buf;

    Master_boot_record* mbr = (Master_boot_record*)buf;

    size_t part_base = lba_to_byte(mbr->p_entry[img->active_partition].lba_first);
    Bios_param_block* bpb = (Bios_param_block*)(buf + part_base);

    /* setting BIOS Parameter Block */
    memcpy(bpb->oem_name, "AXELMOPP", 8);
    bpb->bytes_per_sec = SECTOR_SIZE;
    bpb->sec_per_clus = 1;
    bpb->num_fats = 2;
    bpb->media = 0xf8;

    if (img->fat_type == 32) {
        bpb->root_ent_cnt = 0;
        bpb->total_sec16  = 0;
        bpb->fat_size16   = 0;
        bpb->rsvd_area_sec_num = 32;
        bpb->total_sec32  = mbr->p_entry[img->active_partition].sector_nr;

        /* Calculate the number of cluster. */
        size_t clus_nr = bpb->total_sec32 / bpb->sec_per_clus;
        if (clus_nr < 65525) {
            error_echo("Invalid FAT32 size");
            return EXIT_FAILURE;
        }

        /* Calculate the number of sector per FAT. */
        bpb->fat32.fat_size32 = (uint16_t)((clus_nr * 4u) / SECTOR_SIZE);

        bpb->fat32.rde_clus_num = 2;
        bpb->fat32.fsinfo_sec_num = 1;
        bpb->fat32.bk_boot_sec = 6;
    } else if (img->fat_type == 16){
        bpb->rsvd_area_sec_num = 1;
        /* TODO: */
    } else {
        bpb->rsvd_area_sec_num = 1;
        /* TODO: */
    }

    return 0;
}


static inline int create_image(Image* img) {
    char* b = malloc(img->size * sizeof(char));
    if (b == NULL) {
        error_msg("Alloc failed: size %zu", img->size);
        return EXIT_FAILURE;
    }
    memset(b, 0, img->size * sizeof(char));
    img->buf = b;

    if (load_loader(img) != 0) {
        free(b);
        error_echo("Loading boot loader failed\n");
        return EXIT_FAILURE;
    }

    if (set_fat(img) != 0) {
        free(b);
        error_echo("Setting FAT failed\n");
        return EXIT_FAILURE;
    }

    FILE* fp = fopen(img->name, "w+b");
    if (fp == NULL) {
        return EXIT_FAILURE;
    }
    size_t r = fwrite(b, sizeof(char), img->size, fp);
    fclose(fp);

    free(b);

    return (r < img->size) ? EXIT_FAILURE : 0;
}


static inline int set_options(Image* img, int argc, char* const argv[]) {
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
                img->size = (size_t)t;
                break;
            case 'f':
                t = atoi(optarg);
                if ((t != 12) && (t != 16) && (t != 32)) {
                    error_msg("Invalid FAT type: %d", t);
                    return EXIT_FAILURE;
                }
                img->fat_type = (size_t)t;
                break;
            case 'v':
            case 'h':
                puts(
                    "Image util for Axel.\n"
                    "Usage: aim [-s IMAGE_SIZE MB] TARGET_FILE_NAME MBR FILE\n"
                    "  default values\n"
                    "    IMAGE_SIZE: 64 MB");
                return EXIT_FAILURE;
        }
    }

    /* Deal with non-option arguments here */
    for (size_t i = 0; optind < argc; i++, optind++) {
        char const* t = argv[optind];
        switch (i) {
            case 0:
                img->name = t;
                break;
            case 1:
                img->mbr_path = t;
                break;
            case 2:
                img->second_loader_path = t;
                break;
        }
    }

    if (img->size == 0) {
        img->size = 32 * 1024 * 1024;
    }
    img->size = ALIGN_UP(img->size, SECTOR_SIZE);

    if (img->name == NULL) {
        error_echo("Please input target file path\n");
        return EXIT_FAILURE;
    }

    if (img->mbr_path == NULL) {
        error_echo("Please input MBR file path\n");
        return EXIT_FAILURE;
    }

    if (img->second_loader_path == NULL) {
        error_echo("Please input second loader file path\n");
        return EXIT_FAILURE;
    }

    return 0;
}
