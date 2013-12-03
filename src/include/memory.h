/************************************************************
 * File: include/memory.h
 * Description: about memory header
 ************************************************************/

#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

/* page_table_entry -> page_table -> page_directory_entry -> page_directory_table */

/* ページ一つに相当し、ページについての情報を保持する */
/* 制御レジスタの値によってはCPUに無視されるフラグがある */
struct page_table_entry {
    /* 物理メモリに存在するか否か */
    unsigned int preset_flag                    : 1;
    /* 読み込み専用(0) or 読み書き可能(1) */
    unsigned int read_write_flag                : 1;
    /* ページの権限 */
    unsigned int user_supervisor_flag           : 1;
    /* キャッシュ方式が ライトバック(0), ライトスルー(1) */
    unsigned int page_level_write_throgh_flag   : 1;
    /* キャッシュを有効にするか否か */
    unsigned int page_level_cache_disable_flag  : 1;
    /* ページがアクセスされたか否か */
    unsigned int access_flag                    : 1;
    /* ページが変更されたか否か */
    unsigned int dirty_flag                     : 1;
    /* PAT機能で使用される */
    unsigned int page_attr_table_flag           : 1;
    /* ページがグローバルか否か */
    unsigned int global_flag                    : 1;
    /* CPUに無視されるのでOSが自由に使用してよい */
    unsigned int usable_for_os                  : 3;
    /* ページに割り当てられる物理アドレスの上位20bit */
    unsigned int page_frame_addr                : 20;
};
typedef struct page_table_entry Page_table_entry;
_Static_assert(sizeof(Page_table_entry) == 4, "Static ERROR : Page_table_entry size is NOT 4 byte(32 bit).");


/* ページテーブル一つに相当する */
struct page_directory_entry {
    unsigned int preset_flag                    : 1;
    unsigned int read_write_flag                : 1;
    unsigned int user_supervisor_flag           : 1;
    unsigned int page_level_write_throgh_flag   : 1;
    unsigned int page_level_cache_disable_flag  : 1;
    unsigned int access_flag                    : 1;
    unsigned int reserved                       : 1;
    /* このPDE内のページのサイズを指定する 4KB(0) or 4MB(1)*/
    unsigned int page_size_flag                 : 1;
    unsigned int global_flag                    : 1;
    unsigned int usable_for_os                  : 3;
    /* 管理対象のページテーブルアドレス */
    unsigned int page_table_addr                : 20;
};
typedef struct page_directory_entry Page_directory_entry;
_Static_assert(sizeof(Page_directory_entry) == 4, "Static ERROR : Page_directory_entry size is NOT 4 byte(32 bit).");

enum paging_constants {
    PAGE_SIZE       = 4096,
    PAGE_TABLE_SIZE = 1024,
};

/* the page_table size is 1024(PAGE_TABLE_SIZE) * 4KB(1page) = 4MB */
/* extern Page_table_entry page_table[PAGE_TABLE_SIZE]; */


/* リンカスクリプトで設定される数値を取得 */
uint32_t get_kernel_start_addr(void);
uint32_t get_kernel_end_addr(void);
uint32_t get_kernel_size(void);

/* メモリ管理を行う */
struct memory_manager {
    uint32_t frees, lost_size, lost_times;
    struct  {
        uint32_t base_addr, size;
    } mem_info[1024];
};
typedef struct memory_manager Memory_manager;

#endif
