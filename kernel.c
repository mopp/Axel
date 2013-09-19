/************************************************************
 * File: kernel.c
 * Description: Kernel code
 ************************************************************/

#include <asm_functions.h>
#include <kernel.h>
#include <multiboot_constants.h>
#include <multiboot_structs.h>
#include <graphic_txt.h>

#include <stddef.h>

static Segment_descriptor* set_segment_descriptor(Segment_descriptor*, uint32_t, uint32_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t);
static void init_gdt(void);
static Gate_descriptor* set_gate_descriptor(Gate_descriptor*, uint8_t, uint32_t, uint16_t, uint8_t, uint8_t, uint8_t);
static void init_idt(void);

void kernel_entry(Multiboot_info* boot_info) {
    /* GDT初期化 */
    io_cli();
    init_gdt();
    init_idt();
    /* io_sti(); */

    /* 画面初期化 */
    clean_screen();

    puts("Axel!\n\n");
    puts("Print boot_info\n");

    printf("Boot Loader Name: %s\n", (char*)(boot_info->boot_loader_name));
    printf("%x\n", boot_info->mmap_addr);

    /*
     * Multiboot_memory_map* mmap = (Multiboot_memory_map*)(boot_info->mmap_addr);
     * printf_txt("this is %x\n", mmap->size);
     */

    for(;;) {
        io_hlt();
    };
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
    s->default_op_size = (op_flag != 0) ? (1) : (0);
    /* セグメントサイズを4KB単位とする */
    s->granularity_flag = (g_flag != 0) ? (1) : (0);

    return s;
}


static void init_gdt(void) {
    Segment_descriptor *gdt = (Segment_descriptor*)GDT_ADDR;

    /* 全セグメントを初期化 */
    for (int i = 0; i < GDT_MAX_NUM; ++i) {
        set_segment_descriptor(gdt + i, 0, 0, 0, 0, 0, 0, 0, 0);
    }

    set_segment_descriptor(gdt + 1, 0xffffffff, 0x00000000, GDT_TYPE_CODE_EXR, GDT_TYPE_FOR_CODE_DATA, GDT_RING0, GDT_PRESENT, GDT_DB_OPSIZE_32BIT, GDT_GRANULARITY_4KB);
    set_segment_descriptor(gdt + 2, 0xffffffff, 0x00000000, GDT_TYPE_DATA_RWA, GDT_TYPE_FOR_CODE_DATA, GDT_RING0, GDT_PRESENT, GDT_DB_OPSIZE_32BIT, GDT_GRANULARITY_4KB);

    load_gdtr(GDT_LIMIT, GDT_ADDR);
    change_segment_selectors(0x10);
}


static Gate_descriptor* set_gate_descriptor(Gate_descriptor* g, uint8_t gate_type, uint32_t offset, uint16_t selector_index, uint8_t gate_size, uint8_t pliv, uint8_t p_flag) {
    g->offset_low   = (offset & 0x0000ffff);
    g->offset_high  = (offset >> 16);

    g->segment_selector = selector_index * GDT_ELEMENT_SIZE;

    g->type = gate_type;
    g->size = gate_size;

    g->zero_reserved = 0x00;

    /* 特権レベルを設定 */
    g->plivilege_level = pliv;

    g->present_flag = p_flag;

    return g;
}


static void init_idt(void) {
    Gate_descriptor *idt = (Gate_descriptor*)IDT_ADDR;

    /* 全セグメントを初期化 */
    for (int i = 0; i < IDT_MAX_NUM; ++i) {
        set_gate_descriptor(idt + i, 0, 0, 0, 0, 0, 0);
    }

    /* set_gate_descriptor(idt + 21, IDT_GATE_TYPE_TRAP, (uint32_t)init_idt, 2, IDT_GATE_SIZE_32, IDT_RING0, IDT_PRESENT); */
    load_idtr(IDT_LIMIT, IDT_ADDR);
}
