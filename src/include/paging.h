/**
 * @file paging.c
 * @brief paging header.
 * @author mopp
 * @version 0.1
 * @date 2014-05-20
 */
#ifndef _PAGING_H
#define _PAGING_H
#   ifndef _ASSEMBLY


#include <stdint.h>

/* page_table_entry -> page_table(page_table_entry[1024]) -> page_directory_entry -> page_directory_table(page_directory_entry[1024]) */

/*
 * ページ一つに相当し、ページについての情報を保持する
 * 制御レジスタの値によってはCPUに無視されるフラグがある
 */
struct page_table_entry {
    unsigned int preset_flag : 1;                   /* 物理メモリに存在するか否か */
    unsigned int read_write_flag : 1;               /* 読み込み専用(0) or 読み書き可能(1) */
    unsigned int user_supervisor_flag : 1;          /* ページの権限 */
    unsigned int page_level_write_throgh_flag : 1;  /* キャッシュ方式が ライトバック(0), ライトスルー(1) */
    unsigned int page_level_cache_disable_flag : 1; /* キャッシュを有効にするか否か */
    unsigned int access_flag : 1;                   /* ページがアクセスされたか否か */
    unsigned int dirty_flag : 1;                    /* ページが変更されたか否か */
    unsigned int page_attr_table_flag : 1;          /* PAT機能で使用される */
    unsigned int global_flag : 1;                   /* ページがグローバルか否か */
    unsigned int available_for_os : 3;              /* CPUに無視されるのでOSが自由に使用してよい */
    unsigned int page_frame_addr : 20;              /* ページに割り当てられる物理アドレスの上位20bit */
};
typedef struct page_table_entry Page_table_entry;
_Static_assert(sizeof(Page_table_entry) == 4, "Static ERROR : Page_table_entry size is NOT 4 byte(32 bit).");


/* ページテーブル一つに相当する */
struct page_directory_entry {
    unsigned int preset_flag : 1;
    unsigned int read_write_flag : 1;
    unsigned int user_supervisor_flag : 1;
    unsigned int page_level_write_throgh_flag : 1;
    unsigned int page_level_cache_disable_flag : 1;
    unsigned int access_flag : 1;
    unsigned int reserved : 1;
    unsigned int page_size_flag : 1; /* このPDE内のページのサイズを指定する 4KB(0) or 4MB(1)*/
    unsigned int global_flag : 1;
    unsigned int usable_for_os : 3;
    unsigned int page_table_addr : 20; /* 管理対象のページテーブルアドレス */
};
typedef struct page_directory_entry Page_directory_entry;
_Static_assert(sizeof(Page_directory_entry) == 4, "Static ERROR : Page_directory_entry size is NOT 4 byte(32 bit).");


extern uintptr_t phys_to_vir_addr(uintptr_t);
extern uintptr_t vir_to_phys_addr(uintptr_t);

#define set_phys_to_vir_addr(type, x) (x) = (type)phys_to_vir_addr((x))

/* one page_table should have 1024 page. */
/* one page_directory_table should have 1024 page_table. */

/* the page_table size is 1024(PAGE_NUM) * 4KB(PAGE_SIZE) = 4MB */
/* extern Page_table_entry page_table[PAGE_TABLE_SIZE]; */


#   endif /* _ASSEMBLY */


#define PAGE_SIZE 4096,
#define PAGE_NUM 1024,
#define PAGE_TABLE_NUM 1024,
#define KERNEL_PHYSICAL_BASE_ADDR 0x100000
#define KERNEL_VIRTUAL_BASE_ADDR 0xC0000000



#endif
