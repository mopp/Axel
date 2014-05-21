/************************************************************
 * File: include/kernel.h
 * Description: kernel header
 *              TODO: Add comment
 ************************************************************/

#ifndef KERNEL_H
#define KERNEL_H


#include <stdint.h>

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
_Static_assert(sizeof(Segment_descriptor) == 8, "Static ERROR : Segment_descriptor size is NOT 8 byte(64 bit).");

enum GDT_constants {
    /* 設定先アドレス */
    GDT_ADDR                = 0x00270000 + 0xC0000000,

    GDT_KERNEL_CODE_INDEX   = 1,
    GDT_KERNEL_DATA_INDEX   = 2,

    /* セグメントレジスタは16bitで指す先は8byteの要素が配列として並ぶ */
    GDT_ELEMENT_SIZE        = 8,
    GDT_MAX_NUM             = 8192,
    GDT_LIMIT               = GDT_ELEMENT_SIZE * GDT_MAX_NUM - 1,

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


/* Interrupt Gate Descriptor Table */
struct gate_descriptor {
    /* 割り込みハンドラへのオフセット */
    uint16_t        offset_low;
    /* 割り込みハンドラの属するセグメント CSレジスタへ設定される値 */
    uint16_t        segment_selector;
    /* 3つのゲートディスクリプタ的に0で固定して良さそう */
    uint8_t         unused_zero;
    /* 3つのうちどのゲートディスクリプタか */
    unsigned int    type            : 3;
    unsigned int    size            : 1;
    unsigned int    zero_reserved   : 1;
    unsigned int    plivilege_level : 2;
    unsigned int    present_flag    : 1;
    uint16_t        offset_high;
};
typedef struct gate_descriptor Gate_descriptor;
_Static_assert(sizeof(Gate_descriptor) == 8, "Static ERROR : Gate_descriptor size is NOT 8 byte.(64 bit)");


enum IDT_constants {
    IDT_ADDR                = 0x0026f800 + 0xC0000000,
    IDT_MAX_NUM             = 256,
    IDT_LIMIT               = IDT_MAX_NUM * 8 - 1,

    IDT_GATE_TYPE_TASK      = 0x05,
    IDT_GATE_TYPE_INTERRUPT = 0x06,
    IDT_GATE_TYPE_TRAP      = 0x07,

    IDT_GATE_SIZE_16        = 0x00,
    IDT_GATE_SIZE_32        = 0x01,

    IDT_RING0               = 0x00,
    IDT_RING1               = 0x01,
    IDT_RING2               = 0x02,
    IDT_RING3               = 0x03,

    IDT_PRESENT             = 1,
    IDT_NOT_PRESENT         = 0,
};


/* Programmable Interrupt Controller */
enum PIC_constants {
    /* PIC Port Address */
    PIC0_CMD_STATE_PORT     = 0x20,
    PIC1_CMD_STATE_PORT     = 0xA0,
    PIC0_IMR_DATA_PORT      = 0x21,
    PIC1_IMR_DATA_PORT      = 0xA1,

    /* Initialization Control Words */
    /* PICの設定 */
    PIC0_ICW1               = 0x11,
    /* IRQから割り込みベクタへの変換されるベースの番号
     * x86では3-7bitを使用する
     * この番号にPICのピン番号が加算された値の例外ハンドラが呼び出される
     */
    PIC0_ICW2               = 0x20,
    /* スレーブと接続されているピン */
    PIC0_ICW3               = 0x04,
    /* 動作モードを指定 */
    PIC0_ICW4               = 0x01,

    PIC1_ICW1               = PIC0_ICW1,
    PIC1_ICW2               = 0x28,
    PIC1_ICW3               = 0x02,
    PIC1_ICW4               = PIC0_ICW4,

    /* Operation Command Words2 */
    PIC_OCW2_EOI            = 0x20,

    PIC_IMR_MASK_IRQ0       = 0x01,
    PIC_IMR_MASK_IRQ1       = 0x02,
    PIC_IMR_MASK_IRQ2       = 0x04,
    PIC_IMR_MASK_IRQ3       = 0x08,
    PIC_IMR_MASK_IRQ4       = 0x10,
    PIC_IMR_MASK_IRQ5       = 0x20,
    PIC_IMR_MASK_IRQ6       = 0x40,
    PIC_IMR_MASK_IRQ7       = 0x80,
    PIC_IMR_MASK_IRQ_ALL    = 0xFF,
};


/* Programmable Interval Timer */
enum PIT_constants {
    PIT_PORT_COUNTER0       = 0x40,
    PIT_PORT_COUNTER1       = 0x41,
    PIT_PORT_COUNTER2       = 0x42,
    PIT_PORT_CONTROL        = 0x43,

    /* 制御コマンド */
    /* パルス生成モード */
    PIT_ICW                 = 0x34,

    /* カウンター値 */
    /* 1193182 / 100 Hz */
    PIT_COUNTER_VALUE_HIGH  = 0x2E,
    PIT_COUNTER_VALUE_LOW   = 0x9C,
};


#endif
