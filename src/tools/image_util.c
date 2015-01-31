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
#include <math.h>
#include "../include/fat.h"
#include "../include/fat_manip.h"
#include "../include/macros.h"


#define error_msg(fmt, ...) fprintf(stderr, "ERROR " fmt, __VA_ARGS__)
#define error_echo(fmt) fprintf(stderr, "ERROR " fmt)


enum {
    FAT_AREA_NUM                    = 2,
    BYTE_PER_SECTOR                 = 512,
    SECTOR_PER_CLUSTER              = 1,
    BYTE_PER_CLUSTER                = BYTE_PER_SECTOR * SECTOR_PER_CLUSTER,
    FAT12_RESERVED_AREA_SECTOR_SIZE = 1,
    FAT16_RESERVED_AREA_SECTOR_SIZE = 1,
    FAT32_RESERVED_AREA_SECTOR_SIZE = 7,
    DIR_ENTRY_NUM_IN_ROOT_DIR_TABLE = 64,
};


/* DIR_ENTRY_NUM_IN_ROOT_DIR_TABLE should be an even multiple of BYTE_PER_SECTOR. */
_Static_assert((DIR_ENTRY_NUM_IN_ROOT_DIR_TABLE * 32) % BYTE_PER_SECTOR == 0, "DIR_ENTRY_NUM_IN_ROOT_DIR_TABLE is invalid.");


typedef struct {
    void* img_buffer;
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
static int embed_files_into_image(Fat_image*, char const*);
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
        error_echo("Create image failed.\n");
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
        /* minimum image size = MBR + loader2. */
        min_image_size = BYTE_PER_SECTOR + ALIGN_UP(get_file_size(img->loader2_path), BYTE_PER_SECTOR);
        if (img->manip.fat_type == FAT_TYPE16) {
            /* + reserved area + FAT entry area + root directory entry. */
            min_image_size += (FAT16_RESERVED_AREA_SECTOR_SIZE * BYTE_PER_SECTOR) + (FAT16_MIN_CLUSTER_SIZE * BYTE_PER_CLUSTER) + (2u * (size_t)FAT16_MIN_CLUSTER_SIZE * (size_t)FAT_AREA_NUM) + (DIR_ENTRY_NUM_IN_ROOT_DIR_TABLE * 32);
        } else {
            /* + reserved area + FAT entry area. */
            min_image_size += (size_t)(FAT32_RESERVED_AREA_SECTOR_SIZE * BYTE_PER_SECTOR) + (size_t)(FAT32_MIN_CLUSTER_SIZE * BYTE_PER_CLUSTER) + (4u * (size_t)FAT32_MIN_CLUSTER_SIZE * (size_t)FAT_AREA_NUM);
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
        error_echo("FAT construction failed\n");
        return EXIT_FAILURE;
    }

    /* Make "boot" direction in root directory to store files. */
    char const* dir = "boot";
    uint32_t root_dir_cluster = GET_VALUE_BY_FAT_TYPE(img->manip.fat_type, FAT12_ROOT_DIR_CLUSTER_SIGNATURE, FAT16_ROOT_DIR_CLUSTER_SIGNATURE, img->manip.bpb->fat32.root_dentry_cluster);
    int state = fat_make_directory(&img->manip, root_dir_cluster, dir, DIR_ATTR_READ_ONLY);
    if (state != AXEL_SUCCESS) {
        error_echo("Making boot directory failed\n");
        return EXIT_FAILURE;
    }

    /* Embedded files. */
    if (embed_files_into_image(img, dir) != 0) {
        error_echo("Embedded files failed\n");
        return EXIT_FAILURE;
    }

    /* Create disk image file by image buffer. */
    FILE* fp = fopen(img->img_path, "w+b");
    if (fp == NULL) {
        error_msg("cannot open file %s\n", img->img_path);
        return EXIT_FAILURE;
    }
    size_t written_size = fwrite(img->img_buffer, 1, img->img_size, fp);
    fclose(fp);

    return (written_size < img->img_size) ? (EXIT_FAILURE) : (0);
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
    Master_boot_record* mbr = img->img_buffer;
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
    puts("FAT filesystem construction");
    Partition_entry* fat_partition = &img->mbr->p_entry[img->active_partition];
    uint32_t fat_type = img->manip.fat_type;

    /* Setting BIOS Parameter Block. */
    Bios_param_block* bpb = (Bios_param_block*)((uintptr_t)img->img_buffer + (lba_to_byte(fat_partition->lba_first)));
    memset(bpb, 0, sizeof(Bios_param_block));

    memcpy(bpb->oem_name, "AXELMOPP", 8);
    bpb->bytes_per_sec     = BYTE_PER_SECTOR;
    bpb->sec_per_clus      = SECTOR_PER_CLUSTER;
    bpb->rsvd_area_sec_num = GET_VALUE_BY_FAT_TYPE(fat_type, FAT12_RESERVED_AREA_SECTOR_SIZE, FAT16_RESERVED_AREA_SECTOR_SIZE, FAT32_RESERVED_AREA_SECTOR_SIZE);
    bpb->fat_area_num      = FAT_AREA_NUM;
    bpb->root_ent_cnt      = (fat_type == FAT_TYPE32) ? (0) : (DIR_ENTRY_NUM_IN_ROOT_DIR_TABLE);
    bpb->media             = 0xf8;

    /* Init manipulator. */
    Fat_manips* fm       = &img->manip;
    fm->bpb              = bpb;
    fm->b_access         = block_access;
    fm->alloc            = malloc;
    fm->free             = free;
    fm->obj              = img;
    fm->byte_per_cluster = bpb->sec_per_clus * bpb->bytes_per_sec;

    /*
     * Calculate the number of sectors of FAT area.
     * Reference: "How mformat-3.9.10 and above calculates needed FAT size"
     *      http://www.gnu.org/software/mtools/manual/fat_size_calculation.pdf
     */
    size_t byte_per_sec           = bpb->bytes_per_sec;
    size_t sec_per_clus           = bpb->sec_per_clus;
    size_t fat_area_num           = bpb->fat_area_num;
    size_t remaining_sectors      = fat_partition->sector_nr - bpb->rsvd_area_sec_num - ((32 * bpb->root_ent_cnt + byte_per_sec - 1) / byte_per_sec);
    size_t fat_nubbles_num        = GET_VALUE_BY_FAT_TYPE(fat_type, 3, 4, 8);
    uint16_t one_fat_area_sectors = (uint16_t)ceil(((remaining_sectors + 2 * sec_per_clus) * fat_nubbles_num) / (2 * sec_per_clus * byte_per_sec + fat_area_num * fat_nubbles_num));
    one_fat_area_sectors          = ALIGN_UP(one_fat_area_sectors, BYTE_PER_SECTOR);

    if (fat_type == FAT_TYPE32) {
        bpb->total_sec32               = fat_partition->sector_nr;
        bpb->fat32.fat_sector_size32   = one_fat_area_sectors;
        bpb->fat32.root_dentry_cluster = 2;
        bpb->fat32.fsinfo_sector       = 1;
        bpb->fat32.backup_boot_sector  = 6;

        /* Copy Backup boot sector. */
        void* bpb_buf = malloc(BYTE_PER_SECTOR);
        memset(bpb_buf, 0, BYTE_PER_SECTOR);
        memcpy(bpb_buf, bpb, sizeof(Bios_param_block));
        block_access(img, FILE_WRITE, bpb->fat32.backup_boot_sector, 1, bpb_buf);
        free(bpb_buf);

        fat_calc_sectors(bpb, &fm->area);
        fm->max_fat_entry_number = fm->area.data.sec_nr / bpb->sec_per_clus;

        /* Init FSINFO structure. */
        fm->fsinfo->lead_signature   = FSINFO_LEAD_SIG;
        fm->fsinfo->struct_signature = FSINFO_STRUCT_SIG;
        fm->fsinfo->free_cnt         = fm->max_fat_entry_number - 1;    /* -1 for root directory entry. */
        fm->fsinfo->next_free        = 2;                                                           /* FAT[0], FAT[1] are reserved FAT entry, Thus start point is FAT[2]. */
        fm->fsinfo->trail_signature  = FSINFO_TRAIL_SIG;
        fat_fsinfo_access(fm, FILE_WRITE, fm->fsinfo);

        /* Set root directory FAT entry. */
        set_last_fat_entry(fm, bpb->fat32.root_dentry_cluster);
    } else {
        if (0x10000 <= fat_partition->sector_nr) {
            bpb->total_sec32 = fat_partition->sector_nr;
        } else {
            bpb->total_sec16 = fat_partition->sector_nr & 0xffffu;
        }
        bpb->fat_sector_size16 = one_fat_area_sectors;

        fat_calc_sectors(bpb, &fm->area);
        fm->max_fat_entry_number = fm->area.data.sec_nr / bpb->sec_per_clus;
    }

    puts("\tFAT areas\n\t[Area name]: [Begin sector] - [End sector] [The number of sector]");
    Fat_area* area = &fm->area;
    printf("\tReserved area: %d - %d (%d)\n", area->rsvd.begin_sec, area->rsvd.sec_nr, area->rsvd.sec_nr);
    printf("\tFAT area     : %d - %d (%d)\n", area->fat.begin_sec, area->fat.begin_sec + area->fat.sec_nr, area->fat.sec_nr);
    printf("\tRoot dir area: %d - %d (%d)\n", area->rdentry.begin_sec, area->rdentry.begin_sec + area->rdentry.sec_nr, area->rdentry.sec_nr);
    printf("\tDATA area    : %d - %d (%d)\n", area->data.begin_sec, area->data.begin_sec + area->data.sec_nr, area->data.sec_nr);

    /* Init FAT[0] and FAT[1]. */
    fat_entry_write(fm, 0, 0xFFFFFF00 | bpb->media);
    fat_entry_write(fm, 1, 0xFFFFFFFF);

    return 0;
}


static inline int embed_files_into_image(Fat_image* img, char const* base_dir_name) {
    /* Find directory that are embeddeded files. */
    uint32_t dir_cluster = fat_find_file_cluster(&img->manip, fat_get_root_dir_cluster(&img->manip), base_dir_name);
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


static Axel_state_code block_access(void* p, uint8_t direction, uint32_t lba, uint8_t sec_cnt, uint8_t* buf) {
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
