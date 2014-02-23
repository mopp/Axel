/************************************************************
 * File: include/vbe.h
 * Description: VBE 2.0 Structures
 ************************************************************/

#ifndef VBE_H
#define VBE_H


#include <stdint.h>

typedef struct {
    uint8_t vbe_signature[4];   /* 'VESA' VBE Signature */
    uint16_t vbe_version;       /* BCD value, VBE Version */
    uint32_t oem_string_ptr;    /* Pointer to OME String. */
    uint32_t capabilities;      /* indicate the support of specific features */
    uint32_t video_mode_ptr;    /* VbeFarPtr to supported VideoModeList */
    uint16_t total_memory;      /* Number of 64KB memory blocks */
    uint16_t oem_software_rev;
    uint32_t oem_vendor_name_ptr;
    uint32_t oem_product_name_ptr;
    uint32_t oem_product_rev_ptr;
    uint8_t reserved[220];
    uint8_t oem_data[255];
} Vbe_info_block;
_Static_assert(sizeof(Vbe_info_block) == 512, "Static ERROR : Vbe_info_block size is NOT 512 byte.");

typedef struct {
    uint16_t mode_attributes;
    uint8_t win_a_attributes;
    uint8_t win_b_attributes;
    uint16_t win_granularity;
    uint16_t win_size;
    uint16_t win_a_segment;
    uint16_t win_b_segment;
    uint32_t win_func_ptr;
    uint16_t byte_per_scanline;

    uint16_t x_resolution;
    uint16_t y_resolution;
    uint8_t x_char_size;
    uint8_t y_char_size;
    uint8_t number_of_planes;
    uint8_t bits_per_pixel;
    uint8_t number_of_banks;
    uint8_t memory_model_type;
    uint8_t bank_size;
    uint8_t number_of_image_pages;
    uint8_t reserved_page;

    uint8_t red_mask_size;
    uint8_t red_field_position;
    uint8_t green_mask_size;
    uint8_t green_field_position;
    uint8_t blue_mask_size;
    uint8_t blue_field_position;
    uint8_t rsvd_mask_size;
    uint8_t rsvd_field_position;
    uint8_t direct_color_mode_info;

    uint32_t phys_base_ptr;
    uint32_t reserved_phys1;
    uint16_t reserved_phys2;

    uint16_t linbytes_per_scanline;
    uint8_t bnk_number_of_imagepages;
    uint8_t lin_number_of_imagepages;
    uint8_t lin_red_mask_size;
    uint8_t lin_red_field_position;
    uint8_t lin_green_mask_size;
    uint8_t lin_green_field_position;
    uint8_t lin_blue_mask_size;
    uint8_t lin_blue_field_position;
    uint8_t lin_rsvd_mask_size;
    uint8_t lin_rsvd_field_position;
    uint32_t max_pixel_clock;

    uint8_t reserved[188];
} Vbe_mode_info_block ;
_Static_assert(sizeof(Vbe_mode_info_block ) == 256, "Static ERROR : Vbe_mode_info_block size is NOT 256 byte.");

typedef struct {
    uint32_t signature;
    uint16_t entry_point;
    uint16_t pm_initialize;
    uint16_t bios_data_sel;
    uint16_t a0000_sel;
    uint16_t b0000_sel;
    uint16_t b8000_sel;
    uint16_t code_segment_sel;
    uint8_t in_protect_mode;
    uint8_t checksum;
} Pm_info_block;

#endif
