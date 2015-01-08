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
    char* const* insert_files;
    size_t insert_file_num;
    unsigned char active_partition;
    Fat_manips manip;
} Fat_image;


static int set_options(Fat_image*, int, char* const[]);
static int create_image(Fat_image*);
static int load_loader(Fat_image*);
static int construct_fat(Fat_image*);
static int insert_files_into_image(Fat_image*, char const*);
static Axel_state_code block_access(void*, uint8_t, uint32_t, uint8_t, uint8_t*);
static size_t get_file_size(char const*);
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
    memset(&img->manip, 0, sizeof(Fat_manips));
    img->img_path       = NULL;
    img->mbr_path       = NULL;
    img->loader2_path   = NULL;
    img->img_size       = 0;
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
                    "FAT filesystem image util for Axel.\n"
                    "Usage: img_util [-s IMAGE_SIZE MB] TARGET_FILE_NAME MBR FILE\n"
                    "  default values\n"
                    "    IMAGE_SIZE: 32 MB");
                return EXIT_FAILURE;
        }
    }

    if ((argc - optind) < 3) {
        error_echo("Too few arguments.\n");
        return EXIT_FAILURE;
    }

    /* Deal with non-option arguments. */
    img->img_path        = argv[optind];
    img->mbr_path        = argv[optind + 1];
    img->loader2_path    = argv[optind + 2];

    /* Set remain arguments. */
    img->insert_file_num = ((size_t)argc - (size_t)optind) - 3u;
    if (0 < img->insert_file_num) {
        img->insert_files = &argv[optind + 3];
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

    /* Loading boot loaders. */
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
    puts("Construct FAT filesystem.");

    /* Make "boot" direction in root directory. */
    char const* dir = "boot";
    int state = fat_make_directory(&img->manip, img->manip.bpb->fat32.rde_clus_num, dir, DIR_ATTR_READ_ONLY);
    if (state != AXEL_SUCCESS) {
        error_echo("Making boot directory failed\n");
        return EXIT_FAILURE;
    }

    /* Insert files in arguments. */
    if (insert_files_into_image(img, dir) != 0) {
        error_echo("Inserting files failed\n");
        return EXIT_FAILURE;
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
    size_t tmp = get_file_size(img->mbr_path);
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
    size_t second_loader_size = tmp;
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
    Master_boot_record* mbr = img->mbr;

    /* Setting BIOS Parameter Block */
    Bios_param_block* bpb = (Bios_param_block*)((uintptr_t)img->img_buffer + (lba_to_byte(mbr->p_entry[img->active_partition].lba_first)));
    memcpy(bpb->oem_name, "AXELMOPP", 8);
    bpb->bytes_per_sec = SECTOR_SIZE;
    bpb->sec_per_clus  = 1;
    bpb->num_fats      = 2;
    bpb->media         = 0xf8;

    /* Init manipulator */
    Fat_manips* fm = &img->manip;
    fm->bpb              = bpb;
    fm->b_access         = block_access;
    fm->alloc            = malloc;
    fm->free             = free;
    fm->obj              = img;
    fm->byte_per_cluster = bpb->sec_per_clus * bpb->bytes_per_sec;

    if (img->manip.fat_type == FAT_TYPE32) {
        bpb->root_ent_cnt      = 0;
        bpb->total_sec16       = 0;
        bpb->fat_size16        = 0;
        bpb->rsvd_area_sec_num = 4;
        bpb->total_sec32       = mbr->p_entry[img->active_partition].sector_nr;

        /* Calculate the number of cluster. */
        size_t spc                = bpb->sec_per_clus;
        size_t sectors            = bpb->total_sec32 - bpb->rsvd_area_sec_num;
        size_t fat_entry_nr       = (sectors / spc);
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

        // printf("fat size: %d\n", bpb->fat32.fat_size32);
        // printf("fat entries %d\n", bpb->fat32.fat_size32 * SECTOR_SIZE / 4);
        // printf("Total cluster %d\n", bpb->total_sec32);
        // printf("rsvd begin   : %5d\n", fm->area.rsvd.begin_sec);
        // printf("rsvd    nr   : %5d\n", fm->area.rsvd.sec_nr);
        // printf("fat  begin   : %5d\n", fm->area.fat.begin_sec);
        // printf("fat     nr   : %5d\n", fm->area.fat.sec_nr);
        // printf("rdentry begin: %5d\n", fm->area.rdentry.begin_sec);
        // printf("rdentry    nr: %5d\n", fm->area.rdentry.sec_nr);
        // printf("data begin   : %5d\n", fm->area.data.begin_sec);
        // printf("data    nr   : %5d\n", fm->area.data.sec_nr);

        /* Init FAT[0] and FAT[1]. */
        uint32_t fat_entry = 0xFFFFFF00 | bpb->media;
        fat_entry_access(fm, FILE_WRITE, 0, fat_entry);
        fat_entry = 0xFFFFFFFF;
        fat_entry_access(fm, FILE_WRITE, 1, fat_entry);

        /* Init FSINFO structure. */
        fm->fsinfo->lead_signature   = FSINFO_LEAD_SIG;
        fm->fsinfo->struct_signature = FSINFO_STRUCT_SIG;
        fm->fsinfo->free_cnt         = (uint32_t)(fm->area.data.sec_nr / bpb->sec_per_clus) - 1; /* -1 for root directory entry. */
        fm->fsinfo->next_free        = 2;                                                       /* FAT[0], FAT[1] are reserved FAT entry, Thus start point is FAT[2]. */
        fm->fsinfo->trail_signature  = FSINFO_TRAIL_SIG;
        fat_fsinfo_access(fm, FILE_WRITE, fm->fsinfo);

        /* Init root directory */
        set_last_fat_entry(fm, bpb->fat32.rde_clus_num);
    } else if (img->manip.fat_type == FAT_TYPE16) {
        bpb->rsvd_area_sec_num = 1;
        /* TODO: */
    } else {
        bpb->rsvd_area_sec_num = 1;
        /* TODO: */
    }

    return 0;
}


static inline int insert_files_into_image(Fat_image* img, char const* base_dir_name) {
    /* Find directory that are inserted files. */
    uint32_t dir_cluster = fat_find_file_cluster(&img->manip, img->manip.bpb->fat32.rde_clus_num, base_dir_name);
    if (dir_cluster == 0) {
        return EXIT_FAILURE;
    }

    for (size_t i = 0; i < img->insert_file_num; i++) {
        char* filename = img->insert_files[i];
        size_t file_size = get_file_size(filename);
        void* buffer = malloc(file_size);
        load_file(filename, buffer, file_size);
        Axel_state_code r = fat_create_file(&img->manip, dir_cluster, filename, DIR_ATTR_READ_ONLY | DIR_ATTR_ARCHIVE, buffer, file_size);
        free(buffer);

        printf("\tFile: %s\n", filename);
        printf("\tSize: %zu\n", file_size);
        if (r == AXEL_SUCCESS) {
            puts("\tInsert Success");
        } else {
            error_echo("\tInsert Failed");
        }
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


static inline size_t get_file_size(char const* fname) {
    FILE* fp = fopen(fname, "rb");
    if (fp == NULL) {
        return 0;
    }

    fseek(fp, 0, SEEK_END);
    long s = ftell(fp);

    fclose(fp);

    return (size_t)s;
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
