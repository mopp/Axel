/************************************************************
 * File: kernel.c
 * Description: Kernel code
 ************************************************************/

#include <asm_functions.h>
#include <graphic.h>
#include <interrupt_handler.h>
#include <kernel.h>
#include <keyboard.h>
#include <list.h>
#include <macros.h>
#include <memory.h>
#include <multiboot.h>
#include <paging.h>
#include <point.h>
#include <queue.h>
#include <string.h>
#include <stdio.h>
#include <vbe.h>

static Segment_descriptor* set_segment_descriptor(Segment_descriptor*, uint32_t, uint32_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t);
static inline void init_gdt(void);
static Gate_descriptor* set_gate_descriptor(Gate_descriptor*, uint8_t, void (*)(void), uint16_t, uint8_t, uint8_t, uint8_t);
static inline void init_idt(void);
static inline void init_pic(void);
static inline void init_pit(void);
static inline void clear_bss(void);

_Noreturn void kernel_entry(Multiboot_info* const boot_info) {
    io_cli();

    /* clear bss section of kernel. */
    clear_bss();

    /*
     * Fix multiboot_info address.
     * Bacause it is physical address yet.
     */
    set_phys_to_vir_addr(uint32_t, boot_info->vbe_control_info);
    set_phys_to_vir_addr(uint32_t, boot_info->vbe_mode_info);

    Vbe_info_block* const vbe_info = (Vbe_info_block*)(uintptr_t)boot_info->vbe_control_info;
    Vbe_mode_info_block* const vbe_mode_info = (Vbe_mode_info_block*)(uintptr_t)boot_info->vbe_mode_info;
    uint32_t const boot_flags = boot_info->flags;

    set_phys_to_vir_addr(uint32_t, vbe_info->video_mode_ptr);

    /* check nmap_* field */
    if (boot_flags & 0x20) {
        set_phys_to_vir_addr(uint32_t, boot_info->mmap_addr);
        /* initialize memory. */
        init_memory((Multiboot_memory_map*)(uintptr_t)boot_info->mmap_addr, boot_info->mmap_length);
    }

    init_gdt();
    init_idt();
    init_pic();
    init_pit();
    init_graphic(vbe_info, vbe_mode_info);

    Point2d p0, p1;
    RGB8 c;
    uint32_t const max_x = get_max_x_resolution() - 1;
    uint32_t const max_y = get_max_y_resolution() - 1;

    clean_screen(set_rgb_by_color(&c, 0x3A6EA5));
    test_draw(set_rgb_by_color(&c, 0x3A6EA5));

    fill_rectangle(set_point2d(&p0, 0, max_y - 27), set_point2d(&p1, max_x, max_y - 27), set_rgb_by_color(&c, 0xC6C6C6));
    fill_rectangle(set_point2d(&p0, 0, max_y - 26), set_point2d(&p1, max_x, max_y - 26), set_rgb_by_color(&c, 0xFFFFFF));
    fill_rectangle(set_point2d(&p0, 0, max_y - 25), set_point2d(&p1, max_x, max_y), set_rgb_by_color(&c, 0xC6C6C6));

    fill_rectangle(set_point2d(&p0, 3, max_y - 23), set_point2d(&p1, 59, max_y - 23), set_rgb_by_color(&c, 0xFFFFFF));
    fill_rectangle(set_point2d(&p0, 2, max_y - 23), set_point2d(&p1, 2, max_y - 3), set_rgb_by_color(&c, 0xFFFFFF));
    fill_rectangle(set_point2d(&p0, 3, max_y - 3), set_point2d(&p1, 59, max_y - 3), set_rgb_by_color(&c, 0x848484));
    fill_rectangle(set_point2d(&p0, 59, max_y - 22), set_point2d(&p1, 59, max_y - 4), set_rgb_by_color(&c, 0x848484));
    fill_rectangle(set_point2d(&p0, 2, max_y - 2), set_point2d(&p1, 59, max_y - 2), set_rgb_by_color(&c, 0x000000));
    fill_rectangle(set_point2d(&p0, 60, max_y - 23), set_point2d(&p1, 60, max_y - 2), set_rgb_by_color(&c, 0x000000));

    fill_rectangle(set_point2d(&p0, max_x - 46, max_y - 23), set_point2d(&p1, max_x - 3, max_y - 23), set_rgb_by_color(&c, 0x848484));
    fill_rectangle(set_point2d(&p0, max_x - 46, max_y - 22), set_point2d(&p1, max_x - 46, max_y - 3), set_rgb_by_color(&c, 0x848484));
    fill_rectangle(set_point2d(&p0, max_x - 46, max_y - 2), set_point2d(&p1, max_x - 3, max_y - 2), set_rgb_by_color(&c, 0xFFFFFF));
    fill_rectangle(set_point2d(&p0, max_x - 2, max_y - 23), set_point2d(&p1, max_x - 2, max_y - 2), set_rgb_by_color(&c, 0xFFFFFF));

    /* FIXME: please implement paging
     * if (init_keyboard() == AXEL_FAILED) {
     *     puts_ascii_font("Keyboard initialize failed", &make_point2d(10, 10));
     * }
     */


    io_sti();

    puts("-------------------- Start Axel ! --------------------\n\n");

    printf("kernel size: %dKB\n", get_kernel_size() / 1024);
    printf("kernel start address: %x\n", get_kernel_start_addr());
    printf("kernel end address: %x\n\n", get_kernel_end_addr());

    printf("PIC0 State: %x\n", io_in8(PIC0_CMD_STATE_PORT));
    printf("PIC0 Data: %x\n", io_in8(PIC0_IMR_DATA_PORT));
    printf("PIC1 State: %x\n", io_in8(PIC1_CMD_STATE_PORT));
    printf("PIC1 Data: %x\n\n", io_in8(PIC1_IMR_DATA_PORT));

    printf("BootInfo flags: %x\n", boot_flags); /* 0001 1010 0110 0111 */

    /* mem_*フィールドを確認 */
    if (boot_flags & 0x01) {
        printf("mem_lower(low memory size): %dKB\n", boot_info->mem_lower);
        printf("mem_upper(extends memory size): %dKB\n", boot_info->mem_upper);
    }

    // size_t size = 83200;
    // char* str = (char*)malloc(sizeof(char) * size);
    // for (int i = 0; i < size; i++) {
    //     str[i] = (char)0xAA;
    // }
    // free(str);
    // printf("test malloc %x\n", (uintptr_t)str);

    const uint32_t base_y = 5;
    const uint32_t base_x = get_max_x_resolution();
    enum {
        BUF_SIZE = 20,
    };
    char buf[BUF_SIZE];
    Point2d const p_num_start = {base_x - (BUF_SIZE * 8), base_y};
    Point2d const p_num_end = {base_x, base_y + 13};

    puts_ascii_font("hlt counter: ", &make_point2d(base_x - ((13 + BUF_SIZE) * 8), base_y));

    // List l;
    // list_init(&l, 8, NULL);

    for (int i = 1;; ++i) {
        /* clean drawing area */
        fill_rectangle(&p_num_start, &p_num_end, set_rgb_by_color(&c, 0x3A6EA5));

        itoa(i, buf, 10);
        puts_ascii_font(buf, &p_num_start);

        io_hlt();
    }
}


static Segment_descriptor* set_segment_descriptor(Segment_descriptor* s, uint32_t limit, uint32_t base_addr, uint8_t type, uint8_t type_flag, uint8_t pliv, uint8_t p_flag, uint8_t op_flag, uint8_t g_flag) {
    /* セグメントのサイズを設定 */
    /* granularity_flagが1のときlimit * 4KBがセグメントのサイズになる */
    s->limit_low = (limit & 0x0000ffff);
    s->limit_hi = (uint8_t)((limit >> 16) & 0xf);

    /* セグメントの開始アドレスを設定 */
    s->base_addr_low = (uint16_t)(base_addr & 0x0000ffff);
    s->base_addr_mid = (uint8_t)((base_addr & 0x00ff0000) >> 16);
    s->base_addr_hi = (uint8_t)((base_addr & 0xff000000) >> 24);

    /* アクセス権エラー */
    if (0xf < type) {
        return NULL;
    }
    /* アクセス権を設定 */
    s->type = (uint8_t)(type & 0x0f);

    /* セグメントタイプエラー */
    if (0x1 < type_flag) {
        return NULL;
    }
    /* 0でシステム用, 1でコードかデータ用のセグメントとなる */
    s->segment_type = (uint8_t)(type_flag & 0x01);

    /* 特権レベルエラー */
    if (0x3 < pliv) {
        return NULL;
    }
    /* 特権レベルを設定 */
    s->plivilege_level = (uint8_t)(pliv & 0x03);

    /* メモリに存在する */
    s->present_flag = (uint8_t)(p_flag & 0x01);
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
    Segment_descriptor* gdt = (Segment_descriptor*)GDT_ADDR;

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


static Gate_descriptor* set_gate_descriptor(Gate_descriptor* g, uint8_t gate_type, void (*offset)(void), uint16_t selector_index, uint8_t gate_size, uint8_t pliv, uint8_t p_flag) {
    g->offset_low = ((uintptr_t)offset & 0x0000ffff);
    g->offset_high = (uint16_t)(((uintptr_t)offset >> 16) & 0x0000ffff);

    g->segment_selector = (uint16_t)(selector_index * GDT_ELEMENT_SIZE);

    g->type = (uint8_t)(gate_type & 0x07);
    g->size = (uint8_t)(gate_size & 0x01);

    g->unused_zero = 0x00;
    g->zero_reserved = 0x0;

    /* 特権レベルを設定 */
    g->plivilege_level = (uint8_t)(pliv & 0x01);

    g->present_flag = IS_FLAG_NOT_ZERO(p_flag);

    return g;
}


static inline void init_idt(void) {
    Gate_descriptor* idt = (Gate_descriptor*)IDT_ADDR;

    /* 全ゲートディスクリプタ初期化を初期化 */
    for (int i = 0; i < IDT_MAX_NUM; ++i) {
        set_gate_descriptor(idt + i, 0, 0, 0, 0, 0, 0);
    }
    load_idtr(IDT_LIMIT, IDT_ADDR);

    set_gate_descriptor(idt + 0x20, IDT_GATE_TYPE_INTERRUPT, asm_interrupt_handler0x20, GDT_KERNEL_CODE_INDEX, IDT_GATE_SIZE_32, IDT_RING0, IDT_PRESENT);
    set_gate_descriptor(idt + 0x21, IDT_GATE_TYPE_INTERRUPT, asm_interrupt_handler0x21, GDT_KERNEL_CODE_INDEX, IDT_GATE_SIZE_32, IDT_RING0, IDT_PRESENT);
}


static inline void init_pic(void) {
    /* 全割り込みを受けない */
    io_out8(PIC0_IMR_DATA_PORT, PIC_IMR_MASK_IRQ_ALL);
    io_out8(PIC1_IMR_DATA_PORT, PIC_IMR_MASK_IRQ_ALL);

    /* Master */
    io_out8(PIC0_CMD_STATE_PORT, PIC0_ICW1);
    io_out8(PIC0_IMR_DATA_PORT, PIC0_ICW2);
    io_out8(PIC0_IMR_DATA_PORT, PIC0_ICW3);
    io_out8(PIC0_IMR_DATA_PORT, PIC0_ICW4);

    /* Slave */
    io_out8(PIC1_CMD_STATE_PORT, PIC1_ICW1);
    io_out8(PIC1_IMR_DATA_PORT, PIC1_ICW2);
    io_out8(PIC1_IMR_DATA_PORT, PIC1_ICW3);
    io_out8(PIC1_IMR_DATA_PORT, PIC1_ICW4);

    /* IRQ2以外の割り込みを受け付けない */
    io_out8(PIC0_IMR_DATA_PORT, PIC_IMR_MASK_IRQ_ALL & (~PIC_IMR_MASK_IRQ2));
}


static inline void init_pit(void) {
    io_out8(PIT_PORT_CONTROL, PIT_ICW);
    io_out8(PIT_PORT_COUNTER0, PIT_COUNTER_VALUE_LOW);
    io_out8(PIT_PORT_COUNTER0, PIT_COUNTER_VALUE_HIGH);

    /* PITはタイマーでIRQ0なのでMaster PICのIRQ0を解除 */
    io_out8(PIC0_IMR_DATA_PORT, io_in8(PIC0_IMR_DATA_PORT) & (0xFF & ~PIC_IMR_MASK_IRQ0));
}


static inline void clear_bss(void) {
    extern uintptr_t LD_KERNEL_BSS_START;
    extern uintptr_t LD_KERNEL_BSS_SIZE;

    memset(&LD_KERNEL_BSS_START, 0, (unsigned long)LD_KERNEL_BSS_SIZE);
}
