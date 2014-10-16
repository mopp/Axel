/**
 * @file kernel.c
 * @brief main Axel codes.
 * @author mopp
 * @version 0.1
 * @date 2014-06-06
 */


#include <asm_functions.h>
#include <exception.h>
#include <graphic.h>
#include <interrupt.h>
#include <kernel.h>
#include <macros.h>
#include <memory.h>
#include <multiboot.h>
#include <paging.h>
#include <point.h>
#include <proc.h>
#include <segment.h>
#include <vbe.h>
#include <window.h>
#include <utils.h>
#include <ps2.h>
#include <tlsf.h>
#include <acpi.h>


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
static Window* console;
static char const * prompt = "> ";
#define MAX_CMD_LEN 20
static char cmd[MAX_CMD_LEN + 1];
static size_t cmd_idx = 0 ;


static void draw_desktop(void);
static void decode_key(void);
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
    init_gdt();
    init_idt();
    init_graphic(boot_info);
    if (AXEL_SUCCESS == init_window()) {
        draw_desktop();
    }
    init_acpi();
    init_pic();
    init_pit();
    init_process();

    if (init_keyboard() == AXEL_FAILED) {
        puts("Keyboard initialize failed\n");
    }

    if (init_mouse() == AXEL_FAILED) {
        puts("Mouse initialize failed\n");
    }

    io_sti();

    Window* mouse_win = get_mouse_window();
    Point2d mouse_p = axel_s.mouse->pos = mouse_win->pos;

    for (;;) {
        if ((axel_s.keyboard->enable_keybord == true) && (aqueue_is_empty(&axel_s.keyboard->aqueue) != true)) {
            decode_key();
        }

        if (axel_s.mouse->enable_mouse == true) {
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
}


static inline void draw_desktop(void) {
    Point2d p0, p1;
    RGB8 c;
    int32_t const max_x = get_max_x_resolution();
    int32_t const max_y = get_max_y_resolution();

    /* allocate background. */
    alloc_filled_window(&make_point2d(0, 0), &make_point2d(get_max_x_resolution(), get_max_y_resolution()), set_rgb_by_color(&c, 0x3A6EA5));

    Window* const status_bar = alloc_filled_window(set_point2d(&p0, 0, max_y - 27), set_point2d(&p1, max_x, 27), set_rgb_by_color(&c, 0xC6C6C6));

    /* 60x20 Button */
    window_fill_area(status_bar, set_point2d(&p0, 3,  3), set_point2d(&p1, 60,  1), set_rgb_by_color(&c, 0xFFFFFF));    // above edge.
    window_fill_area(status_bar, set_point2d(&p0, 3, 23), set_point2d(&p1, 60,  2), set_rgb_by_color(&c, 0x848484));    // below edge.
    window_fill_area(status_bar, set_point2d(&p0, 3,  3), set_point2d(&p1,  1, 20), set_rgb_by_color(&c, 0xFFFFFF));    // left edge.
    window_fill_area(status_bar, set_point2d(&p0, 61, 4), set_point2d(&p1,  1, 20), set_rgb_by_color(&c, 0x848484));    // right edge.
    window_fill_area(status_bar, set_point2d(&p0, 62, 4), set_point2d(&p1,  1, 20), set_rgb_by_color(&c, 0x000001));    // right edge.

    int32_t c_height = 400;
    int32_t c_width = 500;
    RGB8 fg = convert_color2RGB8(0x2EFE2E);
    RGB8 bg = convert_color2RGB8(0x151515);
    console = alloc_filled_window(&make_point2d(50, 100), &make_point2d(c_width, c_height), &bg);
    if (console != NULL) {
        window_draw_line(console, set_point2d(&p0, 0, 0), set_point2d(&p1, 0, c_height), set_rgb_by_color(&c, 0xFFFFFF), 1);
        window_draw_line(console, set_point2d(&p0, c_width - 1, 0), set_point2d(&p1, c_width - 1, c_height), set_rgb_by_color(&c, 0xFFFFFF), 1);
        window_draw_line(console, set_point2d(&p0, 0, c_height - 1), set_point2d(&p1, c_width, c_height - 1), set_rgb_by_color(&c, 0xFFFFFF), 1);
        window_fill_area(console, set_point2d(&p0, 0, 0), set_point2d(&p1,  c_width, 10), set_rgb_by_color(&c, 0xC6C6C6));
        window_set_writable(console, &fg, &bg, set_point2d(&p0, 5, 15), set_point2d(&p1, c_width - 5 - 5, c_height - 15 - 5));
    }

    flush_windows();

    puts("                    ___                   __\n");
    puts("                   /   |   _  __  ___    / /\n");
    puts("                  / /| |  | |/_/ / _ \\  / / \n");
    puts("                 / ___ | _>  <  /  __/ / /  \n");
    puts("                /_/  |_|/_/|_|  \\___/ /_/   \n\n");

    puts(prompt);
}


static inline void do_cmd(char const * cmd)  {
    if (strlen(cmd) == 0) {
        puts(prompt);
        return;
    }

    if (strcmp(cmd, "mem") == 0) {
        printf("Total memory : %zu KB\n", KB(get_total_memory_size()));
    } else if (strcmp(cmd, "va") == 0) {
        printf("kernel virtual addr : 0x%08zx - 0x%08zx\n", get_kernel_vir_start_addr(), get_kernel_vir_end_addr());
    } else if (strcmp(cmd, "pa") == 0) {
        printf("kernel physical addr : 0x%08zx - 0x%08zx\n", get_kernel_phys_start_addr(), get_kernel_phys_end_addr());
    } else if (strcmp(cmd, "size") == 0) {
        printf("kernel size        : %zu KB\n", KB(get_kernel_size()));
        printf("kernel static size : %zu KB\n", KB(get_kernel_static_size()));
    } else if (strcmp(cmd, "buddy") == 0) {
        printf("BuddySystem Total    : %zu KB\n", KB(buddy_get_total_memory_size(axel_s.bman)));
        printf("BuddySystem Frame nr : %zu\n", axel_s.bman->total_frame_nr);
        printf("BuddySystem Free     : %zu KB\n", KB(buddy_get_free_memory_size(axel_s.bman)));
        for (size_t i = 0; i < BUDDY_SYSTEM_MAX_ORDER; i++) {
            printf("  Order: %02zu(%05u) - Buddy %02zu nr\n", i, PO2(i), axel_s.bman->free_frame_nr[i]);
        }
    } else if (strcmp(cmd, "tlsf") == 0) {
        printf("Tlsf total_memory_size : %zu KB\n", KB(axel_s.tman->total_memory_size));
        printf("Tlsf free_memory_size  : %zu KB\n", KB(axel_s.tman->free_memory_size));
    } else if (strcmp(cmd, "clear") == 0) {
        window_fill_area(console, &console->wr_begin, &console->wr_size, &console->bg);
        console->wr_pos = console->wr_begin;
    } else if (strcmp(cmd, "exit") == 0) {
        shutdown();
    } else {
        printf("invalid command - %s\n", cmd);
    }

    puts(prompt);
}


static inline void decode_key(void) {
    static uint8_t on_break = false;
    static char const keymap[] = {
        [0x1C] = 'a',
        [0x32] = 'b',
        [0x21] = 'c',
        [0x23] = 'd',
        [0x24] = 'e',
        [0x2B] = 'f',
        [0x34] = 'g',
        [0x33] = 'h',
        [0x43] = 'i',
        [0x3B] = 'j',
        [0x42] = 'k',
        [0x4B] = 'l',
        [0x3A] = 'm',
        [0x31] = 'n',
        [0x44] = 'o',
        [0x4D] = 'p',
        [0x15] = 'q',
        [0x2D] = 'r',
        [0x1B] = 's',
        [0x2C] = 't',
        [0x3C] = 'u',
        [0x2A] = 'v',
        [0x1D] = 'w',
        [0x22] = 'x',
        [0x35] = 'y',
        [0x1A] = 'z',

        [0x45] = '0',
        [0x16] = '1',
        [0x1E] = '2',
        [0x26] = '3',
        [0x25] = '4',
        [0x2E] = '5',
        [0x36] = '6',
        [0x3D] = '7',
        [0x3E] = '8',
        [0x46] = '9',

        [0x0E] = '`',
        [0x4E] = '-',
        [0x55] = '=',
        [0x5D] = '\\',
    };
    static char const keymap_s[] = {
        [0x45] = ')',
        [0x16] = '!',
        [0x1E] = '@',
        [0x26] = '#',
        [0x25] = '$',
        [0x2E] = '%',
        [0x36] = '^',
        [0x3D] = '&',
        [0x3E] = '\'',
        [0x46] = '(',

        [0x0E] = '~',
        [0x4E] = '_',
        [0x55] = '+',
        [0x5D] = '|',
    };

    enum {
        enter     = 0x5a,
        esc       = 0x76,
        backspace = 0x66,
        space     = 0x29,
        tab       = 0x0D,
        caps      = 0x58,
        l_shift   = 0x12,
        l_alt     = 0x11,
        l_ctrl    = 0x14,
        r_shift   = 0x59,
        /* r_alt     = , */
        /* r_ctrl    = , */
        break_code = 0xf0,
    };

    uint8_t* t = aqueue_get_first(&axel_s.keyboard->aqueue);
    uint8_t kc = *t;
    aqueue_delete_first(&axel_s.keyboard->aqueue);

    bool is_mod_key = false;
    switch (kc) {
        case l_shift:
        case r_shift:
            toggle_boolean(axel_s.keyboard->shift_on);
            is_mod_key = true;
            return;
        case l_alt:
            toggle_boolean(axel_s.keyboard->alt_on);
            is_mod_key = true;
            return;
        case l_ctrl:
            toggle_boolean(axel_s.keyboard->ctrl_on);
            is_mod_key = true;
            return;
        case caps:
            toggle_boolean(axel_s.keyboard->enable_caps_lock);
            is_mod_key = true;
            update_keyboard_led();
            return;
    }

    if (on_break == true) {
        on_break = false;
        return;
    }

    if (break_code == kc) {
        on_break = true;
        return;
    }

    if (is_mod_key == true) {
        return;
    }

    char c;
    switch (kc) {
        case enter:
            putchar('\n');
            cmd[cmd_idx++] = '\0';
            do_cmd(cmd);
            cmd_idx = 0;
            break;
        case esc:
            break;
        case backspace:
            putchar('\b');
            if (0 < cmd_idx) {
                cmd_idx--;
            }
            break;
        case space:
            putchar(' ');
            cmd[cmd_idx++] = ' ';
            break;
        case tab:
            putchar('\t');
            break;
        default:
            c = keymap[kc];
            if (axel_s.keyboard->shift_on == true) {
                if ('a' <= c && c <= 'z') {
                    c -= ('a' - 'A');
                } else {
                    c = keymap_s[kc];
                }
            }
            cmd[cmd_idx++] = c;
            if (cmd_idx == MAX_CMD_LEN) {
                cmd_idx = 0;
            }
            putchar(c);
            break;
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
    axel_s.gdt = (Segment_descriptor*)kmalloc_zeroed(sizeof(Segment_descriptor) * SEGMENT_NUM);
    axel_s.tss = (Task_state_segment*)kmalloc_zeroed(sizeof(Task_state_segment));

    axel_s.tss->ss0 = KERNEL_DATA_SEGMENT_SELECTOR;
    axel_s.tss->cs  = KERNEL_CODE_SEGMENT_SELECTOR;
    axel_s.tss->ss  = KERNEL_DATA_SEGMENT_SELECTOR;
    axel_s.tss->es  = KERNEL_DATA_SEGMENT_SELECTOR;
    axel_s.tss->ds  = KERNEL_DATA_SEGMENT_SELECTOR;
    axel_s.tss->gs  = KERNEL_DATA_SEGMENT_SELECTOR;

    /* Setup flat address model. */
    set_segment_descriptor(axel_s.gdt + KERNEL_CODE_SEGMENT_INDEX, 0x00000000, 0xffffffff, GDT_FLAGS_KERNEL_CODE);
    set_segment_descriptor(axel_s.gdt + KERNEL_DATA_SEGMENT_INDEX, 0x00000000, 0xffffffff, GDT_FLAGS_KERNEL_DATA);
    set_segment_descriptor(axel_s.gdt + USER_CODE_SEGMENT_INDEX, 0x00000000, 0xffffffff, GDT_FLAGS_USER_CODE);
    set_segment_descriptor(axel_s.gdt + USER_DATA_SEGMENT_INDEX, 0x00000000, 0xffffffff, GDT_FLAGS_USER_DATA);
    set_segment_descriptor(axel_s.gdt + KERNEL_TSS_SEGMENT_INDEX, (uint32_t)axel_s.tss, sizeof(Task_state_segment) - 1, GDT_FLAGS_TSS);

    load_gdtr(GDT_LIMIT, (uint32_t)axel_s.gdt);
    set_task_register(KERNEL_TSS_SEGMENT_SELECTOR);
}


static inline Gate_descriptor* set_gate_descriptor(Gate_descriptor* g, void* offset, uint16_t selector_index, uint32_t flags) {
    g->bit_expr_high    = flags;
    g->offset_low       = ECAST_UINT16((uintptr_t)offset);
    g->offset_high      = ECAST_UINT16((uintptr_t)offset >> 16);
    g->segment_selector = ECAST_UINT16(selector_index * sizeof(Gate_descriptor));

    return g;
}


static void hlt(Interrupt_frame* ic) {
    /* BOCHS_MAGIC_BREAK(); */
    io_cli();
    DIRECTLY_WRITE_STOP(uintptr_t, KERNEL_VIRTUAL_BASE_ADDR, ic);
}


static inline void init_idt(void) {
    Gate_descriptor* const idt = (Gate_descriptor*)kmalloc(sizeof(Gate_descriptor) * IDT_NUM);

    /* zero clear Gate_descriptor. */
    memset(idt, 0, sizeof(Gate_descriptor) * IDT_NUM);

    set_gate_descriptor(idt + 0x08, hlt,                      KERNEL_CODE_SEGMENT_INDEX, GD_FLAGS_IDT);
    set_gate_descriptor(idt + 0x0D, hlt,                      KERNEL_CODE_SEGMENT_INDEX, GD_FLAGS_IDT);
    set_gate_descriptor(idt + 0x0E, asm_exception_page_fault, KERNEL_CODE_SEGMENT_INDEX, GD_FLAGS_IDT);
    set_gate_descriptor(idt + 0x20, asm_interrupt_timer,      KERNEL_CODE_SEGMENT_INDEX, GD_FLAGS_IDT_TRAP);
    set_gate_descriptor(idt + 0x21, asm_interrupt_keybord,    KERNEL_CODE_SEGMENT_INDEX, GD_FLAGS_IDT);
    set_gate_descriptor(idt + 0x2C, asm_interrupt_mouse,      KERNEL_CODE_SEGMENT_INDEX, GD_FLAGS_IDT);

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
