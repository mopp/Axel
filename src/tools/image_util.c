#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include "../include/fat.h"
#include "../include/macros.h"


#define error_msg(fmt, ...) fprintf(stderr, "ERROR " fmt, __VA_ARGS__)
#define error_echo(fmt) fprintf(stderr, "ERROR " fmt)


enum {
    SECTOR_SIZE = 512,
};


typedef struct {
    uint8_t* tmp_buffer;
    uint8_t* img_buffer;
    size_t img_size;
    Master_boot_record* mbr;
    Bios_param_block* bpb;
    Fsinfo* fsinfo;
    char const* img_path;
    char const* mbr_path;
    char const* loader2_path;
    unsigned char active_partition;
    unsigned char fat_type;
    Fat_area rsvd;
    Fat_area fat;
    Fat_area rdentry;
    Fat_area data;
} Fat_image;


static int set_options(Fat_image*, int, char* const[]);
static int create_image(Fat_image*);


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
static inline int load_loader(Fat_image* img) {
    char* const buf = img->img_buffer;

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
    img->mbr = mbr;
    if (mbr->boot_signature != BOOT_SIGNATURE) {
        error_echo("MBR is invalid.");
        return EXIT_FAILURE;
    }

    /* Load second loader */
    t = get_file_size(img->loader2_path);
    if (t <= 0) {
        error_echo("Second loader size is invalid");
        return EXIT_FAILURE;
    }
    size_t secl_size = (size_t)t;
    r = load_file(img->loader2_path, buf + write_size, secl_size);
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
    mbr->p_entry[0].sector_nr = (img->img_size / SECTOR_SIZE) - mbr->p_entry[0].lba_first;
    img->active_partition = 0;

    return 0;
}


static inline void* access_buf(Fat_image const* img, uint8_t direction, uint32_t lba, uint8_t sec_cnt, uint8_t* buf) {
    /* lba is relative LBA address. */
    void* p = (void*)(img->img_buffer + (img->mbr->p_entry[img->active_partition].lba_first) + (lba * SECTOR_SIZE));
    size_t s = sec_cnt * SECTOR_SIZE;
    if (direction == FILE_READ) {
        return memcpy(buf, p, s);
    } else if (direction == FILE_WRITE) {
        return memcpy(p, buf, s);
    }

    return NULL;
}


static inline uint32_t fat_entry_access(Fat_image const* img, uint8_t direction, uint32_t n, uint32_t write_entry) {
    uint32_t offset       = n * ((img->fat_type == 32) ? 4 : 2);
    uint32_t bps          = img->bpb->bytes_per_sec;
    uint32_t sec_num      = img->bpb->rsvd_area_sec_num + (offset / bps);
    uint32_t entry_offset = offset % bps;

    if (access_buf(img, FILE_READ, sec_num, 1, img->tmp_buffer) == NULL) {
        return 0;
    }
    uint32_t* b = (uint32_t*)(uintptr_t)&img->tmp_buffer[entry_offset];
    uint32_t read_entry = 0;

    /* Filter */
    switch (img->fat_type) {
        case 12:
            read_entry = (*b & 0xFFF);
            break;
        case 16:
            read_entry = (*b & 0xFFFF);
            break;
        case 32:
            read_entry = (*b & 0x0FFFFFFF);
            break;
    }

    if (direction == FILE_READ) {
        return read_entry;
    }

    /* Change FAT entry. */
    switch (img->fat_type) {
        case 12:
            *b = (write_entry & 0xFFF);
            break;
        case 16:
            *b = (write_entry & 0xFFFF);
            break;
        case 32:
            /* keep upper bits */
            *b = (*b & 0xF0000000) | (write_entry & 0x0FFFFFFF);
            break;
    }

    if (access_buf(img, FILE_WRITE, sec_num, 1, img->tmp_buffer) == 0) {
        return 0;
    }

    return *b;
}


static inline void fat_calc_sectors(Fat_image* fm) {
    Bios_param_block* const bpb = fm->bpb;

    /* These are logical sector number. */
    uint32_t fat_size = (bpb->fat_size16 != 0) ? (bpb->fat_size16) : (bpb->fat32.fat_size32);
    uint32_t total_sec = (bpb->total_sec16 != 0) ? (bpb->total_sec16) : (bpb->total_sec32);
    fm->rsvd.begin_sec = 0;
    fm->rsvd.sec_nr = bpb->rsvd_area_sec_num;
    fm->fat.begin_sec = fm->rsvd.begin_sec + fm->rsvd.sec_nr;
    fm->fat.sec_nr = fat_size * bpb->num_fats;
    fm->rdentry.begin_sec = fm->fat.begin_sec + fm->fat.sec_nr;
    fm->rdentry.sec_nr = (32 * bpb->root_ent_cnt + bpb->bytes_per_sec - 1) / bpb->bytes_per_sec;
    fm->data.begin_sec = fm->rdentry.begin_sec + fm->rdentry.sec_nr;
    fm->data.sec_nr = total_sec - fm->data.begin_sec;
}


static inline int set_fat(Fat_image* img) {
    Master_boot_record* mbr = img->mbr;

    Bios_param_block* bpb = (Bios_param_block*)((uintptr_t)img->img_buffer + (lba_to_byte(mbr->p_entry[img->active_partition].lba_first)));
    img->bpb = bpb;

    /* setting BIOS Parameter Block */
    memcpy(bpb->oem_name, "AXELMOPP", 8);
    bpb->bytes_per_sec = SECTOR_SIZE;
    bpb->sec_per_clus = 1;
    bpb->num_fats = 2;
    bpb->media = 0xf8;

    if (img->fat_type == 32) {
        bpb->root_ent_cnt = 0;
        bpb->total_sec16 = 0;
        bpb->fat_size16 = 0;
        bpb->rsvd_area_sec_num = 32;
        bpb->total_sec32 = mbr->p_entry[img->active_partition].sector_nr;

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

        fat_calc_sectors(img);

        printf("rsvd begin   : %5d\n", img->rsvd.begin_sec);
        printf("rsvd    nr   : %5d\n", img->rsvd.sec_nr);
        printf("fat  begin   : %5d\n", img->fat.begin_sec);
        printf("fat     nr   : %5d\n", img->fat.sec_nr);
        printf("rdentry begin: %5d\n", img->rdentry.begin_sec);
        printf("rdentry    nr: %5d\n", img->rdentry.sec_nr);
        printf("data begin   : %5d\n", img->data.begin_sec);
        printf("data    nr   : %5d\n", img->data.sec_nr);
    } else if (img->fat_type == 16) {
        bpb->rsvd_area_sec_num = 1;
        /* TODO: */
    } else {
        bpb->rsvd_area_sec_num = 1;
        /* TODO: */
    }

    return 0;
}


static inline int create_image(Fat_image* img) {
    char* b = malloc(img->img_size * sizeof(char));
    if (b == NULL) {
        error_msg("Alloc failed: size %zu", img->img_size);
        return EXIT_FAILURE;
    }
    img->tmp_buffer = malloc(SECTOR_SIZE * sizeof(char));
    if (img->tmp_buffer == NULL) {
        free(b);
        return EXIT_FAILURE;
    }
    memset(b, 0, img->img_size * sizeof(char));
    img->img_buffer = b;

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

    FILE* fp = fopen(img->img_path, "w+b");
    if (fp == NULL) {
        return EXIT_FAILURE;
    }
    size_t r = fwrite(b, sizeof(char), img->img_size, fp);
    fclose(fp);

    free(b);

    return (r < img->img_size) ? EXIT_FAILURE : 0;
}


static inline int set_options(Fat_image* img, int argc, char* const argv[]) {
    img->img_path = NULL;
    img->mbr_path = NULL;
    img->loader2_path = NULL;
    img->img_size = 0;
    img->fat_type = 32;

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
                img->fat_type = (unsigned char)t;
                break;
            case 'v':
            case 'h':
                puts(
                    "Fat_image util for Axel.\n"
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
        img->img_size = 32 * 1024 * 1024;
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
