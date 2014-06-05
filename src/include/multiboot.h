/**
 * @file include/multiboot.h
 * @brief Multiboot standerd structures and constants.
 * @author mopp
 * @version 0.1
 * @date 2014-05-21
 */

#ifndef _MULTIBOOT_H_
#define _MULTIBOOT_H_

#include <flag.h>

#   ifndef _ASSEMBLY


#include <stdint.h>

/* OSイメージに付属するマルチブートヘッダ */
typedef struct Multiboot_header {
    uint32_t magic;
    uint32_t flags;
    uint32_t checksum;
    /* flagsの16bitが1のとき有効 */
    uint32_t header_addr;
    uint32_t load_addr;
    uint32_t load_end_addr;
    uint32_t bss_end_addr;
    uint32_t entry_addr;
    /* flagsの2bitが1のとき有効 */
    uint32_t mode_type;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
} Multiboot_header;


/* The symbol table for a.out. */
typedef struct {
    uint32_t tabsize;
    uint32_t strsize;
    uint32_t addr;
    uint32_t reserved;
} Multiboot_aout_symbol_table;


/* The section header table for ELF. */
typedef struct {
    uint32_t num;
    uint32_t size;
    uint32_t addr;
    uint32_t shndx;
} Multiboot_elf_section_header_table;


typedef struct {
    /*
     * Multiboot info version number
     * bit00 mem_*
     * bit01 boot_device
     * bit02 cmdline
     * bit03 mods_*
     * bit04 a.outカーネルイメージ用
     * bit05 ELFカーネルイメージ用
     * bit06 mmap_*
     * bit07 drives_*
     * bit08 config_table
     * bit09 boot_loader_name
     * bit10 apm_table
     * bit11 vbe_*
     */
    uint32_t flags;
    /* Available memory from BIOS */
    uint32_t mem_lower;
    uint32_t mem_upper;
    /* "root" partition */
    uint32_t boot_device;
    /* Kernel command line */
    uint32_t cmdline;
    /* Boot-Module list */
    uint32_t mods_count;
    uint32_t mods_addr;
    union {
        Multiboot_aout_symbol_table aout_sym;
        Multiboot_elf_section_header_table elf_sec;
    };
    /* Memory Mapping buff er */
    uint32_t mmap_length;
    uint32_t mmap_addr;
    /* Drive Info buff er */
    uint32_t drives_length;
    uint32_t drives_addr;
    /* ROM confi guration table */
    uint32_t config_table;
    /* Boot Loader Name */
    uint32_t boot_loader_name;
    /* APM table */
    uint32_t apm_table;
    /* Video */
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
} Multiboot_info;


struct multiboot_mmap {
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    /* uint32_t addr_low; */
    /* uint32_t addr_hi; */
    /* uint32_t len_low; */
    /* uint32_t len_hi; */
    uint32_t type;
};
typedef struct multiboot_mmap Multiboot_memory_map;


enum Multiboot_memory_map_constans {
    MULTIBOOT_MEMORY_AVAILABLE = 1,
    MULTIBOOT_MEMORY_RESERVED = 2,
    MMAP_USABLE_MEMORY_FOR_OS       = 0x01,
    MMAP_UNUSABLE_ROM               = 0x02,
    MMAP_USABLE_MEMORY_FOR_ACPI     = 0x03,
    MMAP_UNUSABLE_MEMORY_FOR_ACPI   = 0x04,
};


struct multiboot_mod_list {
    /* the memory used goes from bytes ’mod start’ to ’mod end-1’ inclusive */
    uint32_t mod_start;
    uint32_t mod_end;
    /* Module command line */
    uint32_t cmdline;
    /* padding to take it to 16 bytes (must be zero) */
    uint32_t pad;
};
typedef struct multiboot_mod_list Multiboot_module;


#   endif   /* _ASSEMBLY */


#ifdef TEXT_MODE
    #define GRAPHIC_FIELD_MODE      1
    #define DISPLAY_X_RESOLUTION    320
    #define DISPLAY_Y_RESOLUTION    200
    #define DISPLAY_BIT_SIZE        8
#else
    #define GRAPHIC_FIELD_MODE      0
    #define DISPLAY_X_RESOLUTION    800
    #define DISPLAY_Y_RESOLUTION    600
    #define DISPLAY_BIT_SIZE        32
    /* (800 * 600 * 4) / 1024 = 1875KB = 1.875MB */
#endif
#define DISPLAY_NUMBER_OF_PIXEL (DISPLAY_X_RESOLUTION * DISPLAY_Y_RESOLUTION)

/*
 * Multiboot Header Constants
 */

/* The magic field should contain this. */
#define MULTIBOOT_HEADER_MAGIC      0x1BADB002

/* Align all boot modules on i386 page (4KB) boundaries. */
#define MULTIBOOT_PAGE_ALIGN_BIT    0x00000001

/* Must pass memory information to OS. */
#define MULTIBOOT_MEMORY_INFO_BIT   0x00000002

/* Must pass video information to OS. */
#define MULTIBOOT_VIDEO_MODE_BIT    0x00000004

/* This flag indicates the use of the address fi elds in the header. */
#define MULTIBOOT_AOUT_KLUDGE_BIT   0x00010000


/*
 * Multiboot Info Struct Constants
 */

/* Flags to be set in the 'flags' member of the multiboot info structure. */
/* is there basic lower/upper memory information? */
#define MULTIBOOT_INFO_HAS_MEMORY           0x00000001

/* is there a boot device set? */
#define MULTIBOOT_INFO_HAS_BOOT_DEV         0x00000002

/* is the command-line defi ned? */
#define MULTIBOOT_INFO_HAS_CMDLINE          0x00000004

/* are there modules to do something with? */
#define MULTIBOOT_INFO_HAS_MODS             0x00000008

/* is there a symbol table loaded? */
#define MULTIBOOT_INFO_HAS_AOUT_SYMS        0x00000010

/* is there an ELF section header table? */
#define MULTIBOOT_INFO_HAS_ELF_SYMS         0X00000020

/* is there a full memory map? */
#define MULTIBOOT_INFO_HAS_MEM_MAP          0x00000040

/* Is there drive info? */
#define MULTIBOOT_INFO_HAS_DRIVE_INFO       0x00000080

/* Is there a confi g table? */
#define MULTIBOOT_INFO_HAS_CONFIG_TABLE     0x00000100

/* Is there a boot loader name? */
#define MULTIBOOT_INFO_HAS_BOOT_LOADER_NAME 0x00000200

/* Is there a APM table? */
#define MULTIBOOT_INFO_HAS_APM_TABLE        0x00000400

/* Is there video information? */
#define MULTIBOOT_INFO_HAS_VIDEO_INFO       0x00000800


/*
 * Multiboot Other Constants
 */

/* How many bytes from the start of the file we search for the header. */
#define MULTIBOOT_SEARCH_BYTE       8192

/* This should be in %eax. */
#define MULTIBOOT_BOOTLOADER_MAGIC  0x2BADB002

/* The bits in the required part of flags field we don't support. */
#define MULTIBOOT_UNSUPPORTED       0x0000fffc

/* Alignment of the multiboot info structure. */
#define MULTIBOOT_INFO_ALIGN        0x00000004

/* Alignment of multiboot modules. */
#define MULTIBOOT_MOD_ALIGN         0x00001000


#endif
