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
#include <assert.h>
#include "../include/fat.h"
#include "../include/fat_manip.h"
#include "../include/macros.h"


#define error_msg(fmt, ...) fprintf(stderr, "ERROR " fmt, __VA_ARGS__)
#define error_echo(fmt) fprintf(stderr, "ERROR " fmt)


enum {
    FAT_AREA_NUM              = 2,
    BYTE_PER_SECTOR           = 512,
    SECTOR_PER_CLUSTER        = 1,
    BYTE_PER_CLUSTER          = BYTE_PER_SECTOR * SECTOR_PER_CLUSTER,
    RESERVED_AREA_SECTOR_SIZE = 4,
    ROOT_DIR_TABLE_ENTRY_NUM  = 64,
};
/* ROOT_DIR_TABLE_ENTRY_NUM should be an even multiple of BYTE_PER_SECTOR. */
_Static_assert((ROOT_DIR_TABLE_ENTRY_NUM * 32) % BYTE_PER_SECTOR == 0, "ROOT_DIR_TABLE_ENTRY_NUM is invalid.");


typedef struct {
    uint8_t* img_buffer;
    size_t img_size;
    char const* img_path;
    char const* mbr_path;
    char const* loader2_path;
    char* const* embedded_files;
    size_t embedded_file_num;
    Master_boot_record* mbr;
    uint8_t active_partition;
    Fat_manips manip;
} Fat_image;

typedef int(*Worker_function)(void*, void*);

static int set_options(Fat_image*, int, char* const[]);
static int malloc_work_wrapper(size_t, Worker_function, void*);
static int create_image(void*, void*);
static int set_partition_loader(Fat_image*);
static int construct_fat(Fat_image*);
static int embedded_files_into_image(Fat_image*, char const*);
static Axel_state_code block_access(void*, uint8_t, uint32_t, uint8_t, uint8_t*);
static size_t get_file_size(char const*);
static void* load_file(char const*, void*, size_t);
static size_t lba_to_byte(size_t);


int main(int argc, char* const argv[]) {
    Fat_image img;
    memset(&img, 0, sizeof(Fat_image));
    if (set_options(&img, argc, argv) != 0) {
        return EXIT_FAILURE;
    }

    if (malloc_work_wrapper(img.img_size, create_image, &img) == 0) {
        puts("Create image succeed.");
        return 0;
    } else {
        error_echo("Create image failed.");
        return EXIT_FAILURE;
    }
}


static inline int set_options(Fat_image* img, int argc, char* const argv[]) {
    img->img_path       = NULL;
    img->mbr_path       = NULL;
    img->loader2_path   = NULL;
    img->manip.fat_type = FAT_TYPE12;

    while (1) {
        int opt = getopt(argc, argv, "hvs:f:m:l:t:");

        if (opt == -1) {
            /* getopt() returns -1 when all command line options are parsed. */
            break;
        }

        int t;
        switch (opt) {
            case 's':
                t = atoi(optarg);
                if (t <= 0) {
                    error_msg("Size is invalid: %d\n", t);
                    return EXIT_FAILURE;
                }
                img->img_size = (size_t)t;
                break;
            case 'f':
                t = atoi(optarg);
                switch (t) {
                    case 12:
                        img->manip.fat_type = FAT_TYPE12;
                        break;
                    case 16:
                        img->manip.fat_type = FAT_TYPE16;
                        break;
                    case 32:
                        img->manip.fat_type = FAT_TYPE32;
                        break;
                    default:
                        error_msg("Invalid FAT type: %d\n", t);
                        return EXIT_FAILURE;
                }
                break;
            case 't':
                img->img_path = optarg;
                break;
            case 'm':
                img->mbr_path = optarg;
                break;
            case 'l':
                img->loader2_path = optarg;
                break;
            case 'v':
            case 'h':
                puts("FAT filesystem image util for Axel.\n"
                        "Usage: img_util [-s IMAGE_SIZE MB] [-f FAT_TYPE] -t TARGET_IMAGE_FILE -m MBR -l LOADER2 [EMBEDDED_FILES...]");
                return EXIT_FAILURE;
        }
    }

    /* Check necessary arguments. */
    if (img->img_path == NULL) {
        error_echo("Please input target image file path.\n");
        return EXIT_FAILURE;
    }

    if (img->mbr_path == NULL) {
        error_echo("Please input MBR file path.\n");
        return EXIT_FAILURE;
    }

    if (img->loader2_path == NULL) {
        error_echo("Please input second loader file path.\n");
        return EXIT_FAILURE;
    }

    /*
     * Calculate minimum disk image size for each FAT type.
     * FAT12 - Set floppy image size.
     * FAT16 - Set minimum FAT16 image size.
     * FAT32 - Set minimum FAT32 image size.
     */
    size_t min_image_size;
    if (img->manip.fat_type == FAT_TYPE12) {
        min_image_size = 1440 * 1024;
    } else {
        /* MBR + loader2 + Reserved area. */
        min_image_size = BYTE_PER_SECTOR + get_file_size(img->loader2_path) + (RESERVED_AREA_SECTOR_SIZE * BYTE_PER_SECTOR);
        if (img->manip.fat_type == FAT_TYPE16) {
            /* Adding FAT entry area and root directory entry. */
            min_image_size += (FAT16_MIN_CLUSTER_SIZE * BYTE_PER_CLUSTER) + (2 * FAT16_MIN_CLUSTER_SIZE * FAT_AREA_NUM) + (ROOT_DIR_TABLE_ENTRY_NUM * 32);
        } else {
            /* Adding FAT entry area. */
            min_image_size += (FAT32_MIN_CLUSTER_SIZE * BYTE_PER_CLUSTER) + (4 * FAT32_MIN_CLUSTER_SIZE * FAT_AREA_NUM);
        }

        min_image_size = ALIGN_UP(min_image_size, BYTE_PER_SECTOR);
    }

    /* Check image size. */
    if (img->img_size == 0) {
        img->img_size = min_image_size;
    } else {
        /* Validation FAT image size. */
        img->img_size = ALIGN_UP(img->img_size, BYTE_PER_SECTOR);
        if (img->img_size < min_image_size) {
            error_msg("Image size(%zu) is too small.\n", img->img_size);
            return EXIT_FAILURE;
        }
    }

    assert(ALIGN_UP(img->img_size, BYTE_PER_SECTOR) == img->img_size);
    printf("Image size: %zu KB\n", img->img_size / 1024);

    /* Set embedded files */
    img->embedded_file_num = ((size_t)argc - (size_t)optind);
    if (0 < img->embedded_file_num) {
        img->embedded_files = &argv[optind];
    }

    return 0;
}


static inline int malloc_work_wrapper(size_t alloc_size, Worker_function f, void* object) {
    void* mem = malloc(alloc_size);
    if (mem == NULL) {
        error_msg("Alloc failed: size %zu", alloc_size);
        return EXIT_FAILURE;
    }

    int result = f(mem, object);

    free(mem);
    return result;
}


static inline int create_image(void* buffer, void* object) {
    Fat_image* img = object;
    memset(buffer, 0, img->img_size);
    img->img_buffer = buffer;

    /* Set Partition info and boot loaders. */
    if (set_partition_loader(img) != 0) {
        error_echo("Loading boot loader failed\n");
        return EXIT_FAILURE;
    }
    printf("FAT partition begin at sector %d.\n", img->mbr->p_entry[img->active_partition].lba_first);
    printf("The number of FAT partition sector %d.\n", img->mbr->p_entry[img->active_partition].sector_nr);

    /* Set FSINFO buffer. */
    Fsinfo mem_fsinfo;
    memset(&mem_fsinfo, 0, sizeof(Fsinfo));
    img->manip.fsinfo = &mem_fsinfo;

    if (construct_fat(img) != 0) {
        error_echo("Setting FAT failed\n");
        return EXIT_FAILURE;
    }
    puts("Construct FAT filesystem.");

    /* Make "boot" direction in root directory. */
    char const* dir = "boot";
    int state = fat_make_directory(&img->manip, img->manip.bpb->fat32.rde_clus_num, dir, DIR_ATTR_READ_ONLY);
    if (state != AXEL_SUCCESS) {
        error_echo("Making boot directory failed\n");
        return EXIT_FAILURE;
    }

    /* Embedded files in arguments. */
    if (embedded_files_into_image(img, dir) != 0) {
        error_echo("embeddeding files failed\n");
        return EXIT_FAILURE;
    }

    FILE* fp = fopen(img->img_path, "w+b");
    if (fp == NULL) {
        error_msg("cannot open file %s\n", img->img_path);
        return EXIT_FAILURE;
    }
    size_t written_size = fwrite(img->img_buffer, 1, img->img_size, fp);
    fclose(fp);

    return (written_size < img->img_size) ? EXIT_FAILURE : 0;
}


static inline int set_partition_loader(Fat_image* img) {
    /* Check MBR size */
    size_t file_size = get_file_size(img->mbr_path);
    if (file_size <= 0) {
        error_msg("Invalid MBR file %s", img->mbr_path);
        return EXIT_FAILURE;
    }

    if (file_size != BYTE_PER_SECTOR) {
        error_msg("Sector size(%d) not equals MBR size(%lu)", BYTE_PER_SECTOR, file_size);
        return EXIT_FAILURE;
    }

    /* This indicates tail of wrote area in image buffer. */
    size_t img_buffer_idx = 0;

    /* Load MBR */
    void* result = load_file(img->mbr_path, img->img_buffer, BYTE_PER_SECTOR);
    if (result == NULL) {
        return EXIT_FAILURE;
    }
    img_buffer_idx += BYTE_PER_SECTOR;

    /* Check valid MBR */
    Master_boot_record* mbr = (Master_boot_record*)img->img_buffer;
    img->mbr = mbr;
    if (mbr->boot_signature != BOOT_SIGNATURE) {
        error_echo("MBR is invalid.");
        return EXIT_FAILURE;
    }

    /* Load second loader */
    file_size = get_file_size(img->loader2_path);
    if (file_size <= 0) {
        error_echo("Second loader size is invalid");
        return EXIT_FAILURE;
    }
    size_t second_loader_size = file_size;
    result = load_file(img->loader2_path, img->img_buffer + img_buffer_idx, second_loader_size);
    if (result == NULL) {
        return EXIT_FAILURE;
    }

    /*
     * FAT structure begin at after second loader.
     * Therefore, Roundup second loader size.
     */
    second_loader_size = ALIGN_UP(second_loader_size, BYTE_PER_SECTOR);

    /*
     * Set MBR configure to make first partition.
     * Allocate second loader before first Partition
     */
    img->active_partition                         = 0;
    mbr->p_entry[img->active_partition].bootable  = 0x80;
    mbr->p_entry[img->active_partition].type      = GET_VALUE_BY_FAT_TYPE(img->manip.fat_type, PART_TYPE_FAT12, PART_TYPE_FAT16, PART_TYPE_FAT32_LBA);
    mbr->p_entry[img->active_partition].lba_first = (uint32_t)((BYTE_PER_SECTOR + second_loader_size) / BYTE_PER_SECTOR);
    mbr->p_entry[img->active_partition].sector_nr = (uint32_t)((img->img_size / BYTE_PER_SECTOR) - mbr->p_entry[0].lba_first);

    return 0;
}


static inline int construct_fat(Fat_image* img) {
    Master_boot_record* mbr = img->mbr;

    /* Setting BIOS Parameter Block */
    Bios_param_block* bpb = (Bios_param_block*)((uintptr_t)img->img_buffer + (lba_to_byte(mbr->p_entry[img->active_partition].lba_first)));
    bpb->bytes_per_sec    = BYTE_PER_SECTOR;
    bpb->sec_per_clus     = SECTOR_PER_CLUSTER;
    bpb->fat_area_num     = FAT_AREA_NUM;
    bpb->media            = 0xf8;
    memcpy(bpb->oem_name, "AXELMOPP", 8);

    /* Init manipulator. */
    Fat_manips* fm       = &img->manip;
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
        bpb->rsvd_area_sec_num = RESERVED_AREA_SECTOR_SIZE;
        bpb->total_sec32       = mbr->p_entry[img->active_partition].sector_nr;

        /* Calculate the number of cluster. */
        size_t spc                = bpb->sec_per_clus;
        size_t sectors            = bpb->total_sec32 - bpb->rsvd_area_sec_num;
        size_t fat_entry_nr       = (sectors / spc);
        size_t all_fat_entry_size = ALIGN_UP(fat_entry_nr * 4u, BYTE_PER_SECTOR);

        /* Assume FAT area size. */
        size_t assumed_fat_sectors = (all_fat_entry_size * bpb->fat_area_num) / BYTE_PER_SECTOR;
        fat_entry_nr               = (sectors - assumed_fat_sectors) / spc;
        all_fat_entry_size         = ALIGN_UP(fat_entry_nr * 4u, BYTE_PER_SECTOR);

        /* Calculate actual the number of sector per a FAT. */
        bpb->fat32.fat_size32     = (uint16_t)(all_fat_entry_size / BYTE_PER_SECTOR);
        bpb->fat32.rde_clus_num   = 2;
        bpb->fat32.fsinfo_sec_num = 1;

        fat_calc_sectors(bpb, &fm->area);

        // printf("fat size: %d\n", bpb->fat32.fat_size32);
        // printf("fat entries %d\n", bpb->fat32.fat_size32 * BYTE_PER_SECTOR / 4);
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
        fm->fsinfo->free_cnt         = (uint32_t)(fm->area.data.sec_nr / bpb->sec_per_clus) - 1;    /* -1 for root directory entry. */
        fm->fsinfo->next_free        = 2;                                                           /* FAT[0], FAT[1] are reserved FAT entry, Thus start point is FAT[2]. */
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


static inline int embedded_files_into_image(Fat_image* img, char const* base_dir_name) {
    /* Find directory that are embeddeded files. */
    uint32_t dir_cluster = fat_find_file_cluster(&img->manip, img->manip.bpb->fat32.rde_clus_num, base_dir_name);
    if (dir_cluster == 0) {
        return EXIT_FAILURE;
    }

    for (size_t i = 0; i < img->embedded_file_num; i++) {
        char* filename = img->embedded_files[i];
        size_t file_size = get_file_size(filename);
        void* buffer = malloc(file_size);
        load_file(filename, buffer, file_size);
        Axel_state_code r = fat_create_file(&img->manip, dir_cluster, filename, DIR_ATTR_READ_ONLY | DIR_ATTR_ARCHIVE, buffer, file_size);
        free(buffer);

        printf("\tFile: %s\n", filename);
        printf("\tSize: %zu\n", file_size);
        if (r == AXEL_SUCCESS) {
            puts("\tembedded Success");
        } else {
            error_echo("\tembedded Failed");
        }
    }

    return 0;
}


static inline Axel_state_code block_access(void* p, uint8_t direction, uint32_t lba, uint8_t sec_cnt, uint8_t* buf) {
    Fat_image* ft = p;

    size_t base_lba = ft->mbr->p_entry[ft->active_partition].lba_first;
    size_t offset   = lba_to_byte(base_lba + lba);
    size_t size     = lba_to_byte(sec_cnt);
    void* target    = &ft->img_buffer[offset];

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


static inline void* load_file(char const* file_name, void* buffer, size_t load_size) {
    FILE* fp = fopen(file_name, "r+b");
    if (fp == NULL) {
        return NULL;
    }

    size_t r = fread(buffer, 1, load_size, fp);

    fclose(fp);
    return (r <= 0) ? (NULL) : (buffer);
}


static inline size_t lba_to_byte(size_t nr) {
    return nr * BYTE_PER_SECTOR;
}
