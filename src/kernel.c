/************************************************************
 * File: kernel.c
 * Description: Kernel code
 ************************************************************/

#include <asm_functions.h>
#include <kernel.h>
#include <multiboot_constants.h>
#include <multiboot_structs.h>
#include <graphic_txt.h>
#include <interrupt_handler.h>
#include <stddef.h>
#include <vbe.h>
#include <memory.h>

#define IS_FLAG_NOT_ZERO(x) ((x != 0) ? 1 : 0)

/* &無しで参照させるためにここで別変数に格納 */
extern uint32_t LD_KERNEL_SIZE;
extern uint32_t LD_KERNEL_START;
extern uint32_t LD_KERNEL_END;

static Segment_descriptor* set_segment_descriptor(Segment_descriptor*, uint32_t, uint32_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t);
static inline void init_gdt(void);
static Gate_descriptor* set_gate_descriptor(Gate_descriptor*, uint8_t, void(*)(void), uint16_t, uint8_t, uint8_t, uint8_t);
static inline void init_idt(void);
static inline void init_pic(void);
static inline void init_pit(void);


_Noreturn void kernel_entry(Multiboot_info* boot_info) {
    const uint32_t KERNEL_SIZE = (uint32_t)&LD_KERNEL_SIZE;
    const uint32_t KERNEL_END_ADDR = (uint32_t)&LD_KERNEL_END;
    const uint32_t KERNEL_START_ADDR = (uint32_t)&LD_KERNEL_START;

    io_cli();
    init_gdt();
    init_idt();
    init_pic();
    init_pit();
    io_sti();

    /* 画面初期化 */
    clean_screen();

    puts("-------------------- Start Axel ! --------------------\n\n");

    printf("kernel size: %dKB\n",           KERNEL_SIZE / 1024);
    printf("kernel start address: 0x%x\n",  KERNEL_START_ADDR);
    printf("kernel end address: 0x%x\n\n",  KERNEL_END_ADDR);

    printf("PIC0 State: %x\n",  io_in8(PIC0_CMD_STATE_PORT));
    printf("PIC0 Data: %x\n",   io_in8(PIC0_IMR_DATA_PORT));
    printf("PIC1 State: %x\n",  io_in8(PIC1_CMD_STATE_PORT));
    printf("PIC1 Data: %x\n\n",   io_in8(PIC1_IMR_DATA_PORT));

    uint32_t boot_flags = boot_info->flags;

    printf("BootInfo flags: %x\n", boot_flags); /* 0001 1010 0110 0111 */

    /* mem_*フィールドを確認 */
    if (boot_flags & 0x01) {
        printf("mem_lower(low memory size): %dKB\n", boot_info->mem_lower);
        printf("mem_upper(extends memory size): %dKB\n", boot_info->mem_upper);
    }

    /* nmap_*フィールドを確認 */
    if (boot_flags & 0x20) {
        uint32_t mmap_length = boot_info->mmap_length;
        uint32_t mmap_addr = boot_info->mmap_addr;

        printf("nmap_length %d\n", mmap_length);
        printf("nmap_addr %x\n", mmap_addr);
    }

    for(;;) {
        puts("\n-------------------- hlt ! --------------------\n");
        io_hlt();
    }
}


static Segment_descriptor* set_segment_descriptor(Segment_descriptor* s, uint32_t limit, uint32_t base_addr, uint8_t type, uint8_t type_flag, uint8_t pliv, uint8_t p_flag, uint8_t op_flag, uint8_t g_flag) {
    /* セグメントのサイズを設定 */
    /* granularity_flagが1のときlimit * 4KBがセグメントのサイズになる */
    s->limit_low = (limit & 0x0000ffff);
    s->limit_hi  = (limit >> 16);

    /* セグメントの開始アドレスを設定 */
    s->base_addr_low = (base_addr & 0x0000ffff);
    s->base_addr_mid = (base_addr & 0x00ff0000) >> 16;
    s->base_addr_hi  = (base_addr & 0xff000000) >> 24;

    /* アクセス権エラー */
    if (0xf < type) {
        return NULL;
    }
    /* アクセス権を設定 */
    s->type = type & 0x0f;

    /* セグメントタイプエラー */
    if (0x1 < type_flag) {
        return NULL;
    }
    /* 0でシステム用, 1でコードかデータ用のセグメントとなる */
    s->segment_type = type_flag;

    /* 特権レベルエラー */
    if (0x3 < pliv) {
        return NULL;
    }
    /* 特権レベルを設定 */
    s->plivilege_level = pliv;

    /* メモリに存在する */
    s->present_flag = p_flag;
    /* OSが任意に使用できる */
    s->available = 0;
    /* 0で予約されている */
    s->zero_reserved = 0;
    /* 扱う単位を16bitか32bitか設定 */
    s->default_op_size = IS_FLAG_NOT_ZERO(op_flag);
    /* セグメントサイズを4KB単位とする */
    s->granularity_flag = IS_FLAG_NOT_ZERO(g_flag);

    return s;
}


static inline void init_gdt(void) {
    Segment_descriptor *gdt = (Segment_descriptor*)GDT_ADDR;

    /* 全セグメントを初期化 */
    for (int i = 0; i < GDT_MAX_NUM; ++i) {
        set_segment_descriptor(gdt + i, 0, 0, 0, 0, 0, 0, 0, 0);
    }

    /* 全アドレス空間を指定 */
    /* Flat Setup */
    set_segment_descriptor(gdt + GDT_KERNEL_CODE_INDEX, 0xffffffff, 0x00000000, GDT_TYPE_CODE_EXR, GDT_TYPE_FOR_CODE_DATA, GDT_RING0, GDT_PRESENT, GDT_DB_OPSIZE_32BIT, GDT_GRANULARITY_4KB);
    set_segment_descriptor(gdt + GDT_KERNEL_DATA_INDEX, 0xffffffff, 0x00000000, GDT_TYPE_DATA_RWA, GDT_TYPE_FOR_CODE_DATA, GDT_RING0, GDT_PRESENT, GDT_DB_OPSIZE_32BIT, GDT_GRANULARITY_4KB);

    load_gdtr(GDT_LIMIT, GDT_ADDR);
    change_segment_selectors(GDT_KERNEL_DATA_INDEX * 8);
}


static Gate_descriptor* set_gate_descriptor(Gate_descriptor* g, uint8_t gate_type, void(*offset)(void), uint16_t selector_index, uint8_t gate_size, uint8_t pliv, uint8_t p_flag) {
    g->offset_low   = ((uint32_t)offset & 0x0000ffff);
    g->offset_high  = ((uint32_t)offset >> 16);

    g->segment_selector = selector_index * GDT_ELEMENT_SIZE;

    g->type = gate_type;
    g->size = gate_size;

    g->unused_zero = 0x00;
    g->zero_reserved = 0x0;

    /* 特権レベルを設定 */
    g->plivilege_level = pliv;

    g->present_flag = IS_FLAG_NOT_ZERO(p_flag);

    return g;
}


static void init_idt(void) {
    Gate_descriptor *idt = (Gate_descriptor*)IDT_ADDR;

    /* 全ゲートディスクリプタ初期化を初期化 */
    for (int i = 0; i < IDT_MAX_NUM; ++i) {
        set_gate_descriptor(idt + i, 0, 0, 0, 0, 0, 0);
    }
    load_idtr(IDT_LIMIT, IDT_ADDR);

    set_gate_descriptor(idt + 0x20, IDT_GATE_TYPE_INTERRUPT, asm_interrupt_handler0x20, GDT_KERNEL_CODE_INDEX, IDT_GATE_SIZE_32, IDT_RING0, IDT_PRESENT);
}


static inline void init_pic(void) {
    /* 全割り込みを受けない */
    io_out8(PIC0_IMR_DATA_PORT, PIC_IMR_MASK_IRQ_ALL);
    io_out8(PIC1_IMR_DATA_PORT, PIC_IMR_MASK_IRQ_ALL);

    /* Master */
    io_out8(PIC0_CMD_STATE_PORT, PIC0_ICW1);
    io_out8(PIC0_IMR_DATA_PORT,  PIC0_ICW2);
    io_out8(PIC0_IMR_DATA_PORT,  PIC0_ICW3);
    io_out8(PIC0_IMR_DATA_PORT,  PIC0_ICW4);

    /* Slave */
    io_out8(PIC1_CMD_STATE_PORT, PIC1_ICW1);
    io_out8(PIC1_IMR_DATA_PORT,  PIC1_ICW2);
    io_out8(PIC1_IMR_DATA_PORT,  PIC1_ICW3);
    io_out8(PIC1_IMR_DATA_PORT,  PIC1_ICW4);

    /* IRQ2以外の割り込みを受け付けない */
    io_out8(PIC0_IMR_DATA_PORT, PIC_IMR_MASK_IRQ_ALL & (~PIC_IMR_MASK_IRQ2));
}


static inline void init_pit(void) {
    io_out8(PIT_PORT_CONTROL, PIT_ICW);
    io_out8(PIT_PORT_COUNTER0, PIT_COUNTER_VALUE_LOW);
    io_out8(PIT_PORT_COUNTER0, PIT_COUNTER_VALUE_HIGH);

    /* PITはタイマーでIRQ0なのでPICのIRQ0を解除 */
    /* io_out8(PIC0_IMR_DATA_PORT, io_in8(PIC0_IMR_DATA_PORT) & (~PIC_IMR_MASK_IRQ0)); */
}
