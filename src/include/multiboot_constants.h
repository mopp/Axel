/************************************************************
 * File: include/multiboot_constants.h
 * Description: Multiboot Standerd Constants
 ************************************************************/

#ifndef MULTIBOOT_CONSTANTS_H
#define MULTIBOOT_CONSTANTS_H 1

/* XXX 以降はアセンブラで使うためenum化しないこと */
#define DISPLAY_MAX_X      320
#define DISPLAY_MAX_Y      200
#define DISPLAY_MAX_BIT    8
#define DISPLAY_MAX_XY     (DISPLAY_MAX_X * DISPLAY_MAX_Y)

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
