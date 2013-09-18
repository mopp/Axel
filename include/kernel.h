/************************************************************
 * File: include/kernel.h
 * Description: kernel header
 ************************************************************/

#ifndef KERNEL
#define KERNEL

#include <stdint.h>


enum Textmode_constants {
    TEXTMODE_VRAM_ADDR          = 0x000B8000,
    TEXTMODE_ATTR_FORE_COLOR_B  = 0x0100,
    TEXTMODE_ATTR_FORE_COLOR_G  = 0x0200,
    TEXTMODE_ATTR_FORE_COLOR_R  = 0x0400,
    TEXTMODE_ATTR_FBRI_OR_FONT  = 0x0800,
    TEXTMODE_ATTR_BACK_COLOR_B  = 0x1000,
    TEXTMODE_ATTR_BACK_COLOR_G  = 0x2000,
    TEXTMODE_ATTR_BACK_COLOR_R  = 0x4000,
    TEXTMODE_ATTR_BBRI_OR_BLNK  = 0x8000,
    TEXTMODE_DISPLAY_MAX_Y      = 25,
    TEXTMODE_DISPLAY_MAX_X      = 80,
    TEXTMODE_DISPLAY_MAX_XY     = (TEXTMODE_DISPLAY_MAX_X * TEXTMODE_DISPLAY_MAX_Y),
};


/* Global Segment Descriptor Table */
struct segment_descriptor {
    // セグメントのサイズ
    // lowとhighを合わせて20bit
    // granularity_flagによって解釈が異なる
    uint16_t        limit_low;
    uint16_t        base_addr_low;
    uint8_t         base_addr_mid;
    // セグメントのアクセス権や状態について
    // segment_typeによって役割が変化する
    unsigned int    type                : 4;
    // 0のとき、セグメントはシステム用
    // 1のとき、セグメントはコード、データ用
    unsigned int    segment_type        : 1;
    // 0~3の特権レベルを設定
    unsigned int    plivilege_level     : 2;
    // セグメントがメモリに存在する場合1
    // 存在しない場合は0
    unsigned int    present_flag        : 1;
    unsigned int    limit_hi            : 4;
    // OSが自由に使って良い
    unsigned int    available           : 1;
    // 0で予約されている
    unsigned int    zero_reserved       : 1;
    // 1のとき32bitで0のとき16bit
    unsigned int    default_op_size     : 1;
    // 0ならば1Byte単位で1Byte~1MByteの範囲
    // 1ならば4KB単位で4KB~4GBの範囲
    unsigned int    granularity_flag    : 1;
    uint8_t         base_addr_hi;
};
typedef struct segment_descriptor Segment_descriptor;
_Static_assert(sizeof(Segment_descriptor) == 8, "Static ERROR : Segment_descriptor size is NOT 8 byte.");

enum GDT_constants {
    /* 設定先アドレス */
    GDT_ADDR            = 0x00270000,

    /* セグメントレジスタは16bitで指す先は8byteの要素が配列として並ぶ */
    GDT_ELEMENT_SIZE        = 8,
    GDT_LIMIT               = 0xFFFF,
    GDT_MAX_NUM             = GDT_LIMIT / GDT_ELEMENT_SIZE,

    GDT_TYPE_DATA_R         = 0x00, // Read-Only
    GDT_TYPE_DATA_RA        = 0x01, // Read-Only, accessed
    GDT_TYPE_DATA_RW        = 0x02, // Read/Write
    GDT_TYPE_DATA_RWA       = 0x03, // Read/Write, accessed
    GDT_TYPE_DATA_REP       = 0x04, // Read-Only, expand-down
    GDT_TYPE_DATA_REPA      = 0x05, // Read-Only, expand-down, accessed
    GDT_TYPE_DATA_RWEP      = 0x06, // Read/Write, expand-down
    GDT_TYPE_DATA_RWEPA     = 0x07, // Read/Write, expand-down, accessed
    GDT_TYPE_CODE_EX        = 0x08, // Execute-Only
    GDT_TYPE_CODE_EXA       = 0x09, // Execute-Only, accessed
    GDT_TYPE_CODE_EXR       = 0x0A, // Execute/Read
    GDT_TYPE_CODE_EXRA      = 0x0B, // Execute/Read, accessed
    GDT_TYPE_CODE_EXC       = 0x0C, // Execute-Only, conforming
    GDT_TYPE_CODE_EXCA      = 0x0D, // Execute-Only, conforming, accessed
    GDT_TYPE_CODE_EXRC      = 0x0E, // Execute/Read, conforming
    GDT_TYPE_CODE_EXRCA     = 0x0F, // Execute/Read, conforming, accessed

    GDT_TYPE_FOR_SYSTEM    = 0,
    GDT_TYPE_FOR_CODE_DATA = 1,

    GDT_RING0               = 0x00,
    GDT_RING1               = 0x01,
    GDT_RING2               = 0x02,
    GDT_RING3               = 0x03,

    GDT_PRESENT             = 1,
    GDT_NOT_PRESENT         = 0,

    GDT_DB_OPSIZE_16BIT     = 0,
    GDT_DB_OPSIZE_32BIT     = 1,

    GDT_GRANULARITY_1BYTE   = 0,
    GDT_GRANULARITY_4KB     = 1,
};


/* Interrupt Gate Descriptor Table*/
struct gate_descriptor {
    /* 割り込みハンドラへのオフセット */
    uint16_t        offset_low;
    /* 割り込みハンドラの属するセグメント CSレジスタへ設定される値 */
    uint16_t        segment_selector;
    /* 3つのゲートディスクリプタ的に0で固定して良さそう */
    unsigned int    unused_zero     : 7;
    /* 3つのうちどのゲートディスクリプタか */
    unsigned int    type            : 3;
    unsigned int    size            : 2;
    unsigned int    zero_reserved   : 1;
    unsigned int    plivilege_level : 2;
    unsigned int    present_flag    : 1;
    uint16_t        offset_high;
};
typedef struct gate_descriptor Gate_descriptor;
_Static_assert(sizeof(Gate_descriptor) == 8, "Static ERROR : Gate_descriptor size is NOT 8 byte.");

enum IDT_constants {
    IDT_ADDR                = GDT_ADDR + GDT_LIMIT + 1,
    IDT_MAX_NUM             = 256,
    IDT_LIMIT               = IDT_MAX_NUM * 8,

    IDT_GATE_TYPE_TASK      = 0x05,
    IDT_GATE_TYPE_INTERRUPT = 0x06,
    IDT_GATE_TYPE_TRAP      = 0x07,

    IDT_GATE_SIZE_16 = 0x00,
    IDT_GATE_SIZE_32 = 0x01,

    IDT_RING0               = 0x00,
    IDT_RING1               = 0x01,
    IDT_RING2               = 0x02,
    IDT_RING3               = 0x03,

    IDT_PRESENT             = 1,
    IDT_NOT_PRESENT         = 0,
};

#endif
