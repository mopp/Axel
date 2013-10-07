/************************************************************
 * File: include/multiboot_structs.h
 * Description: Multiboot Standerd Structures
 ************************************************************/

#ifndef MULTIBOOT_STRUCTS_H
#define MULTIBOOT_STRUCTS_H 1

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
    /* Multiboot info version number */
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


struct multiboot_mmap_entry {
    uint32_t size;
    uint64_t addr;
    uint64_t len;
#define MULTIBOOT_MEMORY_AVAILABLE 1
#define MULTIBOOT_MEMORY_RESERVED 2
    uint32_t type;
} __attribute__((packed));
typedef struct multiboot_mmap_entry Multiboot_memory_map;


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


#endif
