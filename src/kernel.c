/**
 * @file kernel.c
 * @brief main Axel codes.
 * @author mopp
 * @version 0.1
 * @date 2014-06-06
 */


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
#include <stdio.h>
#include <string.h>
#include <vbe.h>


/*
 * Global Segment Descriptor
 * Segment limit high + low is 20 bit.
 * Segment base address low + mid + high is 32 bit.
 */
struct segment_descriptor {
    union {
        struct {
            uint16_t limit_low;               /* segment limit low. */
            uint16_t base_addr_low;           /* segment address low. */
            uint8_t base_addr_mid;            /* segment address mid. */
            unsigned int type : 4;            /* this shows segment main configuration. */
            unsigned int segment_type : 1;    /* If 0, segment is system segment, if 1, segment is code or data segment. */
            unsigned int plivilege_level : 2; /* This controle accesse level. */
            unsigned int present : 1;         /* Is it exist on memory */
            unsigned int limit_hi : 4;        /* segment limit high. */
            unsigned int available : 1;       /* OS can use this.  */
            unsigned int zero_reserved : 1;   /* this keeps 0. */
            unsigned int op_size : 1;         /* If 0, 16bit segment, If 1 32 bit segment. */
            unsigned int granularity : 1;     /* If 0 unit is 1Byte, If 1 unit is 4KB */
            uint8_t base_addr_hi;             /* segment address high. */
        };
        struct {
            uint32_t bit_expr_low;
            uint32_t bit_expr_high;
        };
    };
};
typedef struct segment_descriptor Segment_descriptor;
_Static_assert(sizeof(Segment_descriptor) == 8, "Static ERROR : Segment_descriptor size is NOT 8 byte(64 bit).");


enum GDT_constants {
    GDT_ADDR = 0x00270000 + KERNEL_VIRTUAL_BASE_ADDR, /* TODO: dynamic allocated. */

    KERNEL_CODE_SEGMENT_INDEX = 1,
    KERNEL_DATA_SEGMENT_INDEX = 2,

    SEGMENT_NUM = 2 + 1,                                  /* There are "+1" to allocate null descriptor. */
    GDT_LIMIT = sizeof(Segment_descriptor) * SEGMENT_NUM, /* Total Segment_descriptor occuping area size. */
    GDT_FLAG_TYPE_DATA_R = 0x000000,                      /* Read-Only */
    GDT_FLAG_TYPE_DATA_RA = 0x000100,                     /* Read-Only, accessed */
    GDT_FLAG_TYPE_DATA_RW = 0x000200,                     /* Read/Write */
    GDT_FLAG_TYPE_DATA_RWA = 0x000300,                    /* Read/Write, accessed */
    GDT_FLAG_TYPE_DATA_REP = 0x000400,                    /* Read-Only, expand-down */
    GDT_FLAG_TYPE_DATA_REPA = 0x000500,                   /* Read-Only, expand-down, accessed */
    GDT_FLAG_TYPE_DATA_RWEP = 0x000600,                   /* Read/Write, expand-down */
    GDT_FLAG_TYPE_DATA_RWEPA = 0x000700,                  /* Read/Write, expand-down, accessed */
    GDT_FLAG_TYPE_CODE_EX = 0x000800,                     /* Execute-Only */
    GDT_FLAG_TYPE_CODE_EXA = 0x000900,                    /* Execute-Only, accessed */
    GDT_FLAG_TYPE_CODE_EXR = 0x000A00,                    /* Execute/Read */
    GDT_FLAG_TYPE_CODE_EXRA = 0x000B00,                   /* Execute/Read, accessed */
    GDT_FLAG_TYPE_CODE_EXC = 0x000C00,                    /* Execute-Only, conforming */
    GDT_FLAG_TYPE_CODE_EXCA = 0x000D00,                   /* Execute-Only, conforming, accessed */
    GDT_FLAG_TYPE_CODE_EXRC = 0x000E00,                   /* Execute/Read, conforming */
    GDT_FLAG_TYPE_CODE_EXRCA = 0x000F00,                  /* Execute/Read, conforming, accessed */
    GDT_FLAG_CODE_DATA_SEGMENT = 0x001000,
    GDT_FLAG_RING0 = 0x000000,
    GDT_FLAG_RING1 = 0x002000,
    GDT_FLAG_RING2 = 0x004000,
    GDT_FLAG_RING3 = 0x006000,
    GDT_FLAG_PRESENT = 0x008000,
    GDT_FLAG_AVAILABLE = 0x100000,
    GDT_FLAG_OP_SIZE = 0x400000,
    GDT_FLAG_GRANULARIT = 0x800000,
    GDT_FLAGS_KERNEL_CODE = GDT_FLAG_TYPE_CODE_EXR | GDT_FLAG_CODE_DATA_SEGMENT | GDT_FLAG_RING0 | GDT_FLAG_PRESENT | GDT_FLAG_OP_SIZE | GDT_FLAG_GRANULARIT,
    GDT_FLAGS_KERNEL_DATA = GDT_FLAG_TYPE_DATA_RWA | GDT_FLAG_CODE_DATA_SEGMENT | GDT_FLAG_RING0 | GDT_FLAG_PRESENT | GDT_FLAG_OP_SIZE | GDT_FLAG_GRANULARIT,
};


/* Interrupt Gate Descriptor Table */
struct gate_descriptor {
    union {
        struct {
            uint16_t offset_low;       /* 割り込みハンドラへのオフセット */
            uint16_t segment_selector; /* 割り込みハンドラの属するセグメント CSレジスタへ設定される値 */

            uint8_t unused_zero;       /* 3つのゲートディスクリプタ的に0で固定して良さそう */
            unsigned int type : 3;     /* 3つのうちどのゲートディスクリプタか */
            unsigned int size : 1;
            unsigned int zero_reserved : 1;
            unsigned int plivilege_level : 2;
            unsigned int present_flag : 1;
            uint16_t offset_high;
        };
        struct {
            uint32_t bit_expr_low;
            uint32_t bit_expr_high;
        };
    };
};
typedef struct gate_descriptor Gate_descriptor;
_Static_assert(sizeof(Gate_descriptor) == 8, "Static ERROR : Gate_descriptor size is NOT 8 byte.(64 bit)");


enum Gate_descriptor_constants {
    IDT_ADDR = 0x0026F800 + KERNEL_VIRTUAL_BASE_ADDR, /* TODO: dynamic allocated. */
    IDT_MAX_NUM = 256,
    IDT_LIMIT = IDT_MAX_NUM * 8 - 1,

    GD_FLAG_TYPE_TASK      = 0x00000500,
    GD_FLAG_TYPE_INTERRUPT = 0x00000600,
    GD_FLAG_TYPE_TRAP      = 0x00000700,
    GD_FLAG_SIZE_32        = 0x00000800,
    GD_FLAG_RING0          = 0x00000000,
    GD_FLAG_RING1          = 0x00002000,
    GD_FLAG_RING2          = 0x00004000,
    GD_FLAG_RING3          = 0x00006000,
    GD_FLAG_PRESENT        = 0x00008000,
    GD_FLAGS_IDT           = GD_FLAG_TYPE_INTERRUPT | GD_FLAG_SIZE_32 | GD_FLAG_RING0 | GD_FLAG_PRESENT,
};


/* Programmable Interval Timer */
enum PIT_constants {
    PIT_PORT_COUNTER0 = 0x40,
    PIT_PORT_COUNTER1 = 0x41,
    PIT_PORT_COUNTER2 = 0x42,
    PIT_PORT_CONTROL  = 0x43,

    /* 制御コマンド */
    /* パルス生成モード */
    PIT_ICW = 0x34,

    /* カウンター値 */
    /* 1193182 / 100 Hz */
    PIT_COUNTER_VALUE_HIGH = 0x2E,
    PIT_COUNTER_VALUE_LOW  = 0x9C,
};


static Segment_descriptor* set_segment_descriptor(Segment_descriptor*, uint32_t, uint32_t, uint32_t);
static Gate_descriptor* set_gate_descriptor(Gate_descriptor*, void*, uint16_t, uint32_t);
static void init_gdt(void);
static void init_idt(void);
static void init_pic(void);
static void init_pit(void);
static void clear_bss(void);



/**
 * @brief This function is start entry.
 *      Initialize system in this.
 * @param boot_info boot information by bootstraps loader.
 */
_Noreturn void kernel_entry(Multiboot_info* const boot_info) {
    io_cli();    /* disable Interrupt until setting Interrupt handler. */
    clear_bss(); /* clear bss section of kernel. */
    Multiboot_info_flag const flags = boot_info->flags;

    if (flags.is_mmap_enable) {
        init_memory(boot_info);
    } else {
        /* TODO: panic */
    }

    init_gdt();
    init_idt();
    init_graphic(boot_info);
    init_pic();
    init_pit();
    io_sti();

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

    puts("-------------------- Start Axel ! --------------------\n\n");

    printf("kernel size: %zuKB\n", get_kernel_size() / 1024);
    printf("kernel static size: %zuKB\n", get_kernel_static_size() / 1024);
    printf("kernel virtual  address: 0x%zx - 0x%zx\n", get_kernel_vir_start_addr(), get_kernel_vir_end_addr());
    printf("kernel physical address: 0x%zx - 0x%zx\n", get_kernel_phys_start_addr(), get_kernel_phys_end_addr());

    if (flags.is_mem_enable) {
        printf("mem_lower(low memory size): %dKB\n", boot_info->mem_lower);
        printf("mem_upper(extends memory size): %dKB\n", boot_info->mem_upper);
        printf("Total memory size: %dKB\n", boot_info->mem_upper + boot_info->mem_lower);
    }

    const uint32_t base_y = 5;
    const uint32_t base_x = get_max_x_resolution();
    enum {
        BUF_SIZE = 20,
    };
    char buf[BUF_SIZE];
    Point2d const p_num_start = {base_x - (BUF_SIZE * 8), base_y};
    Point2d const p_num_end = {base_x, base_y + 13};

    puts_ascii_font("hlt counter: ", &make_point2d(base_x - ((13 + BUF_SIZE) * 8), base_y));

    for (int i = 1;; ++i) {
        /* clean drawing area */
        fill_rectangle(&p_num_start, &p_num_end, set_rgb_by_color(&c, 0x3A6EA5));

        itoa(i, buf, 10);
        puts_ascii_font(buf, &p_num_start);

        io_hlt();
    }
}


static inline Segment_descriptor* set_segment_descriptor(Segment_descriptor* s, uint32_t base_addr, uint32_t limit, uint32_t flags) {
    s->bit_expr_high = flags;

    s->limit_low = ECAST_UINT16(limit);
    s->limit_hi = (limit >> 16) & 0xF;

    s->base_addr_low = ECAST_UINT16(base_addr);
    s->base_addr_mid = ECAST_UINT8(base_addr >> 16);
    s->base_addr_hi = ECAST_UINT8(base_addr >> 24);

    return s;
}


static inline void init_gdt(void) {
    Segment_descriptor* gdt = (Segment_descriptor*)GDT_ADDR;

    /* zero clear Segment_descriptor. */
    memset(gdt, 0, sizeof(Segment_descriptor) * SEGMENT_NUM);

    /* Setup flat address */
    set_segment_descriptor(gdt + KERNEL_CODE_SEGMENT_INDEX, 0x00000000, 0xffffffff, GDT_FLAGS_KERNEL_CODE);
    set_segment_descriptor(gdt + KERNEL_DATA_SEGMENT_INDEX, 0x00000000, 0xffffffff, GDT_FLAGS_KERNEL_DATA);

    load_gdtr(GDT_LIMIT, GDT_ADDR);
}


static inline Gate_descriptor* set_gate_descriptor(Gate_descriptor* g, void* offset, uint16_t selector_index, uint32_t flags) {
    g->bit_expr_high = flags;

    g->offset_low = ECAST_UINT16((uintptr_t)offset);
    g->offset_high = ECAST_UINT16((uintptr_t)offset >> 16);
    g->segment_selector = ECAST_UINT16(selector_index * sizeof(Segment_descriptor));

    return g;
}


static inline void init_idt(void) {
    Gate_descriptor* idt = (Gate_descriptor*)IDT_ADDR;

    /* zero clear Gate_descriptor. */
    memset(idt, 0, sizeof(Gate_descriptor) * IDT_MAX_NUM);

    set_gate_descriptor(idt + 0x0E, io_hlt,                    KERNEL_CODE_SEGMENT_INDEX, GD_FLAGS_IDT);
    set_gate_descriptor(idt + 0x20, asm_interrupt_handler0x20, KERNEL_CODE_SEGMENT_INDEX, GD_FLAGS_IDT);
    set_gate_descriptor(idt + 0x21, asm_interrupt_handler0x21, KERNEL_CODE_SEGMENT_INDEX, GD_FLAGS_IDT);

    load_idtr(IDT_LIMIT, IDT_ADDR);
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
    extern uintptr_t const LD_KERNEL_BSS_START;
    extern uintptr_t const LD_KERNEL_BSS_SIZE;

    memset((void*)&LD_KERNEL_BSS_START, 0, (size_t)&LD_KERNEL_BSS_SIZE);
}
