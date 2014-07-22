/**
 * @file kernel.c
 * @brief main Axel codes.
 * @author mopp
 * @version 0.1
 * @date 2014-06-06
 */


#include <asm_functions.h>
#include <graphic.h>
#include <interrupt.h>
#include <kernel.h>
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
#include <font.h>
#include <window.h>
#include <proc.h>
#include <segment.h>


/*
 * Global Segment Descriptor
 * Segment limit high + low is 20 bit.
 * Segment base address low + mid + high is 32 bit.
 */
union segment_descriptor {
    struct {
        uint16_t limit_low;               /* segment limit low. */
        uint16_t base_addr_low;           /* segment address low. */
        uint8_t base_addr_mid;            /* segment address mid. */
        unsigned int type : 4;            /* this shows segment main configuration. */
        unsigned int segment_type : 1;    /* If 0, segment is system segment, if 1, segment is code or data segment. */
        unsigned int plivilege_level : 2; /* This controles accesse level. */
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
typedef union segment_descriptor Segment_descriptor;
_Static_assert(sizeof(Segment_descriptor) == 8, "Static ERROR : Segment_descriptor size is NOT 8 byte(64 bit).");


enum GDT_constants {
    SEGMENT_NUM                = 4 + 1, /* There are "+1" to allocate null descriptor. */
    GDT_LIMIT                  = sizeof(Segment_descriptor) * SEGMENT_NUM,  /* Total Segment_descriptor occuping area size. */
    GDT_FLAG_TYPE_DATA_R       = 0x000000,  /* Read-Only */
    GDT_FLAG_TYPE_DATA_RA      = 0x000100,  /* Read-Only, accessed */
    GDT_FLAG_TYPE_DATA_RW      = 0x000200,  /* Read/Write */
    GDT_FLAG_TYPE_DATA_RWA     = 0x000300,  /* Read/Write, accessed */
    GDT_FLAG_TYPE_DATA_REP     = 0x000400,  /* Read-Only, expand-down */
    GDT_FLAG_TYPE_DATA_REPA    = 0x000500,  /* Read-Only, expand-down, accessed */
    GDT_FLAG_TYPE_DATA_RWEP    = 0x000600,  /* Read/Write, expand-down */
    GDT_FLAG_TYPE_DATA_RWEPA   = 0x000700,  /* Read/Write, expand-down, accessed */
    GDT_FLAG_TYPE_CODE_EX      = 0x000800,  /* Execute-Only */
    GDT_FLAG_TYPE_CODE_EXA     = 0x000900,  /* Execute-Only, accessed */
    GDT_FLAG_TYPE_CODE_EXR     = 0x000A00,  /* Execute/Read */
    GDT_FLAG_TYPE_CODE_EXRA    = 0x000B00,  /* Execute/Read, accessed */
    GDT_FLAG_TYPE_CODE_EXC     = 0x000C00,  /* Execute-Only, conforming */
    GDT_FLAG_TYPE_CODE_EXCA    = 0x000D00,  /* Execute-Only, conforming, accessed */
    GDT_FLAG_TYPE_CODE_EXRC    = 0x000E00,  /* Execute/Read, conforming */
    GDT_FLAG_TYPE_CODE_EXRCA   = 0x000F00,  /* Execute/Read, conforming, accessed */
    GDT_FLAG_CODE_DATA_SEGMENT = 0x001000,
    GDT_FLAG_RING0             = 0x000000,
    GDT_FLAG_RING1             = 0x002000,
    GDT_FLAG_RING2             = 0x004000,
    GDT_FLAG_RING3             = 0x006000,
    GDT_FLAG_PRESENT           = 0x008000,
    GDT_FLAG_AVAILABLE         = 0x100000,
    GDT_FLAG_OP_SIZE           = 0x400000,
    GDT_FLAG_GRANULARIT        = 0x800000,
    GDT_FLAGS_KERNEL_CODE      = GDT_FLAG_TYPE_CODE_EXR | GDT_FLAG_CODE_DATA_SEGMENT | GDT_FLAG_RING0 | GDT_FLAG_PRESENT | GDT_FLAG_OP_SIZE | GDT_FLAG_GRANULARIT,
    GDT_FLAGS_KERNEL_DATA      = GDT_FLAG_TYPE_DATA_RWA | GDT_FLAG_CODE_DATA_SEGMENT | GDT_FLAG_RING0 | GDT_FLAG_PRESENT | GDT_FLAG_OP_SIZE | GDT_FLAG_GRANULARIT,
    GDT_FLAGS_USER_CODE        = GDT_FLAG_TYPE_CODE_EXR | GDT_FLAG_CODE_DATA_SEGMENT | GDT_FLAG_RING3 | GDT_FLAG_PRESENT | GDT_FLAG_OP_SIZE | GDT_FLAG_GRANULARIT,
    GDT_FLAGS_USER_DATA        = GDT_FLAG_TYPE_DATA_RWA | GDT_FLAG_CODE_DATA_SEGMENT | GDT_FLAG_RING3 | GDT_FLAG_PRESENT | GDT_FLAG_OP_SIZE | GDT_FLAG_GRANULARIT,
};


/* Interrupt Gate Descriptor Table */
union gate_descriptor {
    struct {
        uint16_t offset_low;              /* address offset low of interrupt handler.*/
        uint16_t segment_selector;        /* segment selector register(CS, DS, SS and etc) value. */
        uint8_t unused_zero;              /* unused area. */
        unsigned int type : 3;            /* gate type. */
        unsigned int size : 1;            /* If this is 1, gate size is 32 bit. otherwise it is 16 bit */
        unsigned int zero_reserved : 1;   /* reserved area. */
        unsigned int plivilege_level : 2; /* This controles accesse level. */
        unsigned int present_flag : 1;    /* Is it exist on memory */
        uint16_t offset_high;             /* address offset high of interrupt handler.*/
    };
    struct {
        uint32_t bit_expr_low;
        uint32_t bit_expr_high;
    };
};
typedef union gate_descriptor Gate_descriptor;
_Static_assert(sizeof(Gate_descriptor) == 8, "Static ERROR : Gate_descriptor size is NOT 8 byte.(64 bit)");


enum Gate_descriptor_constants {
    IDT_NUM                = 19 + 12 + 16, /* default interrupt vector + reserved interrupt vector + PIC interrupt vector */
    IDT_LIMIT              = IDT_NUM * 8 - 1,
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
    GD_FLAGS_IDT_TRAP      = GD_FLAG_TYPE_TRAP | GD_FLAG_SIZE_32 | GD_FLAG_RING0 | GD_FLAG_PRESENT,
};


/* Programmable Interval Timer */
enum PIT_constants {
    PIT_PORT_COUNTER0 = 0x40,
    PIT_PORT_COUNTER1 = 0x41,
    PIT_PORT_COUNTER2 = 0x42,
    PIT_PORT_CONTROL = 0x43,

    /* 制御コマンド */
    /* パルス生成モード */
    PIT_ICW = 0x34,

    /* カウンター値 */
    /* 1193182 / 100 Hz */
    PIT_COUNTER_VALUE_HIGH = 0x2E,
    PIT_COUNTER_VALUE_LOW = 0x9C,
};


Axel_struct axel_s;


static void decode_mouse(void);
static Segment_descriptor* set_segment_descriptor(Segment_descriptor*, uint32_t, uint32_t, uint32_t);
static Gate_descriptor* set_gate_descriptor(Gate_descriptor*, void*, uint16_t, uint32_t);
static void init_gdt(void);
static void init_idt(void);
static void init_pit(void);
static void clear_bss(void);



/**
 * @brief This function is start entry.
 *        Initialize system in this.
 * @param boot_info boot information by bootstraps loader.
 */
_Noreturn void kernel_entry(Multiboot_info* const boot_info) {
    io_cli();    /* Disable interrupt until interrupt handler is set. */
    clear_bss(); /* Clear bss section of kernel. */
    Multiboot_info_flag const flags = boot_info->flags;

    if (flags.is_mmap_enable) {
        init_memory(boot_info);
    } else {
        /* TODO: panic */
    }

    init_graphic(boot_info);
    init_gdt();
    init_idt();
    init_process();
    init_pic();
    init_pit();
    io_sti();

    if (AXEL_SUCCESS != init_window()) {
        /* TODO: panic */
    }

    Point2d p0, p1;
    RGB8 c;
    int32_t const max_x = get_max_x_resolution();
    int32_t const max_y = get_max_y_resolution();

    /* allocate background. */
    alloc_filled_window(&make_point2d(0, 0), &make_point2d(get_max_x_resolution(), get_max_y_resolution()), 0, set_rgb_by_color(&c, 0x3A6EA5));

    Window* const status_bar = alloc_filled_window(set_point2d(&p0, 0, max_y - 27), set_point2d(&p1, max_x, 27), 0, set_rgb_by_color(&c, 0xC6C6C6));

    /* 60x20 Button */
    window_fill_area(status_bar, set_point2d(&p0, 3,  3), set_point2d(&p1, 60,  1), set_rgb_by_color(&c, 0xFFFFFF));    // above edge.
    window_fill_area(status_bar, set_point2d(&p0, 3, 23), set_point2d(&p1, 60,  2), set_rgb_by_color(&c, 0x848484));    // below edge.
    window_fill_area(status_bar, set_point2d(&p0, 3,  3), set_point2d(&p1,  1, 20), set_rgb_by_color(&c, 0xFFFFFF));    // left edge.
    window_fill_area(status_bar, set_point2d(&p0, 61, 4), set_point2d(&p1,  1, 20), set_rgb_by_color(&c, 0x848484));    // right edge.
    window_fill_area(status_bar, set_point2d(&p0, 62, 4), set_point2d(&p1,  1, 20), set_rgb_by_color(&c, 0x000001));    // right edge.
    Window* const console = alloc_filled_window(set_point2d(&p0, 400, 100), set_point2d(&p1, 100, 100), 0, set_rgb_by_color(&c,0xec6d71));
    window_draw_line(console, set_point2d(&p0, 1, 1), set_point2d(&p1, 10, 1), set_rgb_by_color(&c, 0x19448e), 5);
    window_draw_line(console, set_point2d(&p0, 10, 10), set_point2d(&p1, 10, 100), set_rgb_by_color(&c, 0x19448e), 10);
    flush_windows();

    if (init_keyboard() == AXEL_FAILED) {
        puts("=Keyboard initialize failed=\n");
    }
    if (init_mouse() == AXEL_FAILED) {
        puts("=Mouse initialize failed=\n");
    }
    Window* mouse_win = get_mouse_window();
    Point2d mouse_p = axel_s.mouse->pos = mouse_win->pos;

    puts("-------------------- Start Axel ! --------------------\n\n");

    printf("kernel size          : %zuKB\n", get_kernel_size() / 1024);
    printf("kernel static size   : %zuKB\n", get_kernel_static_size() / 1024);
    printf("kernel virtual  addr : 0x%zx - 0x%zx\n", get_kernel_vir_start_addr(), get_kernel_vir_end_addr());
    printf("kernel physical addr : 0x%zx - 0x%zx\n", get_kernel_phys_start_addr(), get_kernel_phys_end_addr());
    printf("GDT addr             : 0x%zx\n", (size_t)axel_s.gdt);

    if (flags.is_mem_enable) {
        printf("mem_lower(low memory size): %dKB\n", boot_info->mem_lower);
        printf("mem_upper(extends memory size): %dKB\n", boot_info->mem_upper);
        printf("Total memory size: %dKB\n", boot_info->mem_upper + boot_info->mem_lower);
    }

    /* print_vmem(); */
    /* print_pmem(); */

    for (;;) {
        if (aqueue_is_empty(&axel_s.keyboard->aqueue) != true) {
            aqueue_delete_first(&axel_s.keyboard->aqueue);
        }

        if (aqueue_is_empty(&axel_s.mouse->aqueue) != true) {
            decode_mouse();
        } else if (axel_s.mouse->is_pos_update == false) {
            io_hlt();
        } else {
            Point2d const p = axel_s.mouse->pos;
            move_window(mouse_win, &make_point2d(p.x - mouse_p.x, p.y - mouse_p.y));
            mouse_p = axel_s.mouse->pos;
            axel_s.mouse->is_pos_update = false;
        }
    }
}


static inline void decode_mouse(void) {
    static bool is_discard = false; /* If this is true, discard entire packets. */
    static uint8_t discard_cnt = 0;

    uint8_t packet = *(uint8_t*)aqueue_get_first(&axel_s.mouse->aqueue);
    aqueue_delete_first(&axel_s.mouse->aqueue);

    if (is_discard == true) {
        if (2 < ++discard_cnt) {
            is_discard = false;
            discard_cnt = 0;
        }
        return;
    }

    switch (axel_s.mouse->phase) {
        case 0:
            /*
             * bit meaning of first packet is here.
             * [Y overflow][X overflow][Y sign bit][X sign bit][Always 1][Middle button][Right button][Left button]
             */
            if ((packet & 0x08) == 0) {
                /*
                 * Check fourth bit.
                 * This bit is always 1.
                 * Otherwise, happen anything error.
                 */
                break;
            }

            /*
             * Overflow bits is set.
             * This means that X and Y movement is overflow.
             * So, discard entire packets.
             */
            if ((packet & 0xC0) != 0) {
                is_discard = true;
                break;
            }

            axel_s.mouse->phase = 1;
            axel_s.mouse->packets[0] = packet;

            break;
        case 1:
            /* X axis movement. */
            axel_s.mouse->phase = 2;
            axel_s.mouse->packets[1] = packet;

            break;
        case 2:
            /* Y axis movement. */
            axel_s.mouse->phase = 0;
            axel_s.mouse->packets[2] = packet;

            /*
             * If value is negative, do sign extension.
             * Y direction is inverse of direction.
             */
            int32_t dx = axel_s.mouse->packets[1];
            if ((axel_s.mouse->packets[0] & 0x10) != 0) {
                dx |= 0xFFFFFF00;
            }

            int32_t dy = axel_s.mouse->packets[2];
            if ((axel_s.mouse->packets[0] & 0x20) != 0) {
                dy |= 0xFFFFFF00;
            }

            axel_s.mouse->pos.x += dx;
            axel_s.mouse->pos.y += (~dy + 1);
            axel_s.mouse->button = (axel_s.mouse->packets[0] & 0x07);
            axel_s.mouse->is_pos_update = true;

            break;
        default:
            /* ERROR */
            break;
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
    axel_s.gdt = (Segment_descriptor*)vmalloc(sizeof(Segment_descriptor) * SEGMENT_NUM);

    /* zero clear Segment_descriptor. */
    memset(axel_s.gdt, 0, sizeof(Segment_descriptor) * SEGMENT_NUM);

    /* Setup flat address model. */
    set_segment_descriptor(axel_s.gdt + KERNEL_CODE_SEGMENT_INDEX, 0x00000000, 0xffffffff, GDT_FLAGS_KERNEL_CODE);
    set_segment_descriptor(axel_s.gdt + KERNEL_DATA_SEGMENT_INDEX, 0x00000000, 0xffffffff, GDT_FLAGS_KERNEL_DATA);
    set_segment_descriptor(axel_s.gdt + USER_CODE_SEGMENT_INDEX,   0x00000000, 0xffffffff, GDT_FLAGS_USER_CODE);
    set_segment_descriptor(axel_s.gdt + USER_DATA_SEGMENT_INDEX,   0x00000000, 0xffffffff, GDT_FLAGS_USER_DATA);

    load_gdtr(GDT_LIMIT, (uint32_t)(uintptr_t)axel_s.gdt);
}


static inline Gate_descriptor* set_gate_descriptor(Gate_descriptor* g, void* offset, uint16_t selector_index, uint32_t flags) {
    g->bit_expr_high = flags;

    g->offset_low = ECAST_UINT16((uintptr_t)offset);
    g->offset_high = ECAST_UINT16((uintptr_t)offset >> 16);
    g->segment_selector = ECAST_UINT16(selector_index * sizeof(Gate_descriptor));

    return g;
}


static void hlt(void) {
    io_cli();
    *(uint32_t*)(KERNEL_VIRTUAL_BASE_ADDR) = 0xf5f5f5f5;
    while (1) {
        io_hlt();
    }
}


static inline void init_idt(void) {
    Gate_descriptor* const idt = (Gate_descriptor*)vmalloc(sizeof(Gate_descriptor) * IDT_NUM);

    /* zero clear Gate_descriptor. */
    memset(idt, 0, sizeof(Gate_descriptor) * IDT_NUM);

    set_gate_descriptor(idt + 13,   hlt,                   KERNEL_CODE_SEGMENT_INDEX, GD_FLAGS_IDT);
    set_gate_descriptor(idt + 0x0E, hlt,                   KERNEL_CODE_SEGMENT_INDEX, GD_FLAGS_IDT);
    set_gate_descriptor(idt + 0x20, asm_interrupt_timer,   KERNEL_CODE_SEGMENT_INDEX, GD_FLAGS_IDT_TRAP);
    set_gate_descriptor(idt + 0x21, asm_interrupt_keybord, KERNEL_CODE_SEGMENT_INDEX, GD_FLAGS_IDT);
    set_gate_descriptor(idt + 0x2C, asm_interrupt_mouse,   KERNEL_CODE_SEGMENT_INDEX, GD_FLAGS_IDT);

    load_idtr(IDT_LIMIT, (uint32_t)idt);
}


static inline void init_pit(void) {
    io_out8(PIT_PORT_CONTROL, PIT_ICW);
    io_out8(PIT_PORT_COUNTER0, PIT_COUNTER_VALUE_LOW);
    io_out8(PIT_PORT_COUNTER0, PIT_COUNTER_VALUE_HIGH);

    enable_interrupt(PIC_IMR_MASK_IRQ00);
}


static inline void clear_bss(void) {
    extern uintptr_t const LD_KERNEL_BSS_START;
    extern uintptr_t const LD_KERNEL_BSS_SIZE;

    memset((void*)&LD_KERNEL_BSS_START, 0, (size_t)&LD_KERNEL_BSS_SIZE);
}
