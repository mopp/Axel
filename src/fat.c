/**
 * @file fat.c
 * @brief fat file system implementation.
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
    uint8_t jmp_boot[3];
    uint8_t oem_name[8];
    uint16_t byts_per_sec;
    uint8_t sec_per_clus;
    uint16_t rsvd_sec_cnt;
    uint8_t num_fats;
    uint16_t root_ent_cnt;
    uint16_t total_sec16;
    uint8_t media;
    uint16_t fat_size16;
    uint16_t sec_per_trk;
    uint16_t num_heads;
    uint32_t hidd_sec;
    uint32_t total_sec32;
    union {
        /* FAT12 or FAT16 */
        struct {
            uint8_t drive_num;
            uint8_t reserved_1;
            uint8_t boot_sig;
            uint32_t volume_id;
            uint8_t volume_label[11];
            uint8_t fs_type[8];
        } fat16;
        struct {
            uint16_t fat_size32;
            uint16_t ext_flags;
            uint16_t fat32_version;
            uint32_t root_clustor_num;
            uint16_t fs_info_sec_nun;
            uint16_t bk_boot_sec;
            uint8_t reserved[12];
            uint8_t drive_num;
            uint8_t reserved1;
            uint8_t boot_sig;
            uint32_t volume_id;
            uint8_t volume_label[11];
            uint8_t fs_type[8];
        } fat32;
    };
};
typedef struct bios_param_block Bios_param_block;



static uint8_t* buffer;
static size_t buffer_size;
Axel_state_code init_fat(File_system const* fs) {
    if (fs == NULL) {
        return AXEL_FAILED;
    }

    buffer_size = fs->dev->sector_size * 5;
    buffer = kmalloc(buffer_size);
    FAILED_RETURN(ata_access(ATA_READ, fs->dev, fs->pe->lba_first, 1, buffer));

    /* LBA = (H + C * Total Head size) * (Sector size per track) + (S - 1) */
    /* head     0x00 0000 0000 */
    /* sector   0x21 10 0001 */
    /* cylinder 0x02 000000 0010 */
    /* (0 + 2 * 16) * 63 + (33 - 1) = 2048 = 0x800 */

    Bios_param_block* bpb = (Bios_param_block*)(uintptr_t)buffer;
    printf("%d\n", fs->pe->lba_first);
    printf("%s\n", bpb->oem_name);
    printf("0x%x\n", bpb->fat32.boot_sig);
    printf("0x%x\n", buffer[510]);
    printf("0x%x\n", buffer[511]);

    return AXEL_SUCCESS;
}
