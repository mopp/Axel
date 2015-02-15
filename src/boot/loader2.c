/**
 * @file loader2.c
 * @brief Second OS boot loader
 * @author mopp
 * @version 0.1
 * @date 2015-02-03
 */

#include <stdint.h>
#include <stddef.h>
#include <interrupt.h>
#include <asm_functions.h>
#include <segment.h>
#include <time.h>
#include <macros.h>
#include <stdbool.h>


enum {
    LOADER2_ADDR  = 0x7C00,
    FDD_LOAD_AREA = 0x0500,
};


enum fdd_constants {
    FD_MAX_RETRY_NUM               = 512,
    FD_SECTOR_SIZE                 = 512,
    FD_SECTOR_PER_TRACK            = 18,
    FD_HEAD                        = 2,
    FD_DRIVE0                      = 0,
    FD_DRIVE1                      = 1,
    FD_DRIVE2                      = 2,
    FD_DRIVE3                      = 3,
    STATUS_REGISTER_A              = 0x3F0, /* Read-only */
    STATUS_REGISTER_B              = 0x3F1, /* Read-only */
    DIGITAL_OUTPUT_REGISTER        = 0x3F2,
    TAPE_DRIVE_REGISTER            = 0x3F3,
    MAIN_STATUS_REGISTER           = 0x3F4, /* Read-only */
    DATARATE_SELECT_REGISTER       = 0x3F4, /* Write-only */
    DATA_FIFO_REGISTER             = 0x3F5,
    DIGITAL_INPUT_REGISTER         = 0x3F7, /* Read-only */
    CONFIGURATION_CONTROL_REGISTER = 0x3F7, /* Write-only */
    READ_RACK                      = 0x02,  /* Generates IRQ6 */
    SPECIFY                        = 0x03,  /* Set drive parameters */
    SENSE_DRIVE_STATUS             = 0x04,  /* Get drive status */
    WRITE_DATA                     = 0x05,  /* Write to the disk */
    READ_DATA                      = 0x06,  /* Read from the disk */
    RECALIBRATE                    = 0x07,  /* Seek to track 0  */
    SENSE_INTERRUPT                = 0x08,  /* ack IRQ6, get status of last command */
    WRITE_DELETED_DATA             = 0x09,
    READ_ID                        = 0x0A,  /* Get head location, it generates IRQ6 */
    READ_DELETED_DATA              = 0x0C,
    FORMAT_TRACK                   = 0x0D,
    DUMPREG                        = 0x0E,  /* Get register value for debuging. */
    SEEK                           = 0x0F,  /* Seek heads to the other track. */
    VERSION                        = 0x10,
    SCAN_EQUAL                     = 0x11,
    PERPENDICULAR_MODE             = 0x12,
    CONFIGURE                      = 0x13,  /* Set controller parameters */
    LOCK                           = 0x14,  /* Protect controller params from a reset */
    VERIFY                         = 0x16,
    SCAN_LOW_OR_EQUAL              = 0x19,
    SCAN_HIGH_OR_EQUAL             = 0x1D,
    RELATIVE_SEEK                  = 0x8F,
};
typedef enum fdd_constants Fdd_constants;


struct fd_status_register_a {
    union {
        struct {
            uint8_t head_directon:1;
            uint8_t write_protect:1;
            uint8_t index:1;
            uint8_t head:1;
            uint8_t is_track0:1;
            uint8_t step:1;
            uint8_t has_second_drive:1;
            uint8_t interrupt_pending:1;
        };
        uint8_t bit_expr;
    };
};
typedef struct fd_status_register_a Fd_status_register_a;
_Static_assert(sizeof(Fd_status_register_a) == 1, "");


struct fd_status_register_b {
    union {
        struct {
            uint8_t enable_motor0:1;
            uint8_t enable_motor1:1;
            uint8_t enable_write:1;
            uint8_t read_data_toggle:1;
            uint8_t write_data_toggle:1;
            uint8_t drive_selector0:1;
            uint8_t reserved:2;
        };
        uint8_t bit_expr;
    };
};
typedef struct fd_status_register_b Fd_status_register_b;
_Static_assert(sizeof(Fd_status_register_b) == 1, "");


struct fd_datarate_select_register {
    union {
        struct  {
            uint8_t datarate_selector:2;
            uint8_t pre_complement:3;
            uint8_t reserved:1;
            uint8_t power_down:1;
            uint8_t software_reset:1;
        };
        uint8_t bit_expr;
    };
};
typedef struct fd_datarate_select_register Fd_datarate_select_register;
_Static_assert(sizeof(Fd_datarate_select_register) == 1, "");


/*
 * enable_motorX is exclusive.
 * And We should choose same drive number and motor number.
 */
struct fd_digital_output_register {
    union {
        struct {
            uint8_t drive_selector:2;
            uint8_t not_reset:1; /* If this is 0,  FDD enters reset mode, Otherwise, FDD does normal operation. */
            uint8_t enable_iqr_dma:1;
            uint8_t enable_motor0:1;
            uint8_t enable_motor1:1;
            uint8_t enable_motor2:1;
            uint8_t enable_motor3:1;
        };
        uint8_t bit_expr;
    };
};
typedef struct fd_digital_output_register Fd_digital_output_register;
_Static_assert(sizeof(Fd_digital_output_register) == 1, "");


struct fd_main_status_register {
    union {
        struct {
            uint8_t is_drive0_busy:1;
            uint8_t is_drive1_busy:1;
            uint8_t is_drive2_busy:1;
            uint8_t is_drive3_busy:1;
            uint8_t is_command_busy:1;
            uint8_t non_dma:1;
            uint8_t dio:1;
            uint8_t rqm:1;
        };
        uint8_t bit_expr;
    };
};
typedef struct fd_main_status_register Fd_main_status_register;
_Static_assert(sizeof(Fd_main_status_register) == 1, "");


struct fd_status_register_st0 {
    union {
        struct {
            uint8_t selected_drive:2;
            uint8_t selected_head:1;
            uint8_t reserved:1;
            uint8_t unit_check:1;
            uint8_t is_seek_finish:1;
            uint8_t interrupt_code:2;
        };
        uint8_t bit_expr;
    };
};
typedef struct fd_status_register_st0 Fd_status_register_st0;
_Static_assert(sizeof(Fd_status_register_st0) == 1, "");


struct fd_status_register_st1 {
    union {
        struct {
            uint8_t is_missing_addr_mark:1;
            uint8_t write_disable:1;
            uint8_t no_data:1;
            uint8_t reserved0:1;
            uint8_t is_over_or_under_run:1;
            uint8_t reserved1:1;
            uint8_t is_detect_cylinder_limit:1;
        };
        uint8_t bit_expr;
    };
};
typedef struct fd_status_register_st1 Fd_status_register_st1;
_Static_assert(sizeof(Fd_status_register_st1) == 1, "");


struct fd_status_register_st2 {
    union {
        struct {
            uint8_t missing_data_addr_mark:1;
            uint8_t bad_cylinder:1;
            uint8_t not_hit_scan_condition:1;
            uint8_t hit_scan_condition:1;
            uint8_t error_cylinder:1;
            uint8_t data_field_crc_error:1;
            uint8_t control_mark:1;
            uint8_t reserved:1;
        };
        uint8_t bit_expr;
    };
};
typedef struct fd_status_register_st2 Fd_status_register_st2;
_Static_assert(sizeof(Fd_status_register_st2) == 1, "");


struct fd_status_register_st3 {
    union {
        struct {
            uint8_t selected_drive:2;
            uint8_t head_addr:1;
            uint8_t reserved0:1;
            uint8_t track0:1;
            uint8_t reserved1:1;
            uint8_t write_protect:1;
            uint8_t reserved2:1;
        };
        uint8_t bit_expr;
    };
};
typedef struct fd_status_register_st3 Fd_status_register_st3;
_Static_assert(sizeof(Fd_status_register_st3) == 1, "");


struct fd_dev {
    bool is_initialize;
    bool enable_motor;
    Fdd_constants drive_number;
};
typedef struct fd_dev Fd_dev;


struct memory_info {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
    uint32_t acpi_attr;
};
typedef struct memory_info_e820 Memory_info;


static bool interrupt_fdd_iqr;
static _Noreturn void no_op(void);
static void set_idt(Gate_descriptor*, size_t);
static void clear_bss(void);
static void fill_screen(uint8_t);
static Axel_state_code fd_init(Fd_dev*);
static Axel_state_code fd_cmd_seek(Fd_dev* , uint8_t , uint8_t );
static void fd_set_motor(Fd_dev*, bool);
static Axel_state_code fd_block_access(void* p, uint8_t direction, uint32_t lba, uint8_t sec_cnt, uint8_t* buffer);


_Noreturn void loader2(Memory_info* infos, uint32_t size, uint32_t loader2_size, uint16_t drive_number) {
    io_cli();
    /* Set FS, GS segment. */
    __asm__ volatile(
            "mov $0x10, %ax \n"
            "mov %ax, %fs\n"
            "mov %ax, %gs\n"
            );
    clear_bss();
    fill_screen(0x1);

    init_idt(set_idt, (uintptr_t)no_op);
    init_pit();
    io_sti();

    Fd_dev fdd;
    if (fd_init(&fdd) != AXEL_SUCCESS) {
        fill_screen(0x4);
    }

    uint8_t buffer[FD_SECTOR_SIZE];
    if (fd_block_access(&fdd, 0, 0, 1, buffer) != AXEL_SUCCESS) {
        fill_screen(0x5);
        INF_LOOP();
    }

    fd_set_motor(&fdd, false);

    uint8_t color = 0xF;
    for (;;) {
        wait(500);
        fill_screen(color++ & 0xF);
#if 1
        __asm__ volatile(
                "movl %%eax, %%esi\n"
                :
                : "a"(buffer)
                :
                );
#endif
    }
}


static void fd_set_motor(Fd_dev* fdd, bool is_on) {
    if (is_on == fdd->enable_motor) {
        return;
    }

    Fd_digital_output_register dor = { .bit_expr = 0, };
    dor.drive_selector = (fdd->drive_number) & 0xF;
    dor.not_reset = 1;
    uint8_t flag = (is_on == true) ? (1) : (0);
    switch (fdd->drive_number) {
        case FD_DRIVE0:
            dor.enable_motor0 = flag;
            break;
        case FD_DRIVE1:
            dor.enable_motor1 = flag;
            break;
        case FD_DRIVE2:
            dor.enable_motor2 = flag;
            break;
        case FD_DRIVE3:
            dor.enable_motor3 = flag;
            break;
        default:
            break;
    }

    io_out8(DIGITAL_OUTPUT_REGISTER, dor.bit_expr);

    wait(300);
    fdd->enable_motor = is_on;
}


static Fd_main_status_register fd_read_main_status_register(void) {
    Fd_main_status_register msr;
    msr.bit_expr = io_in8(MAIN_STATUS_REGISTER);
    return msr;
}


static Axel_state_code fd_wait_busy(Fdd_constants drive_number) {
    Fd_main_status_register msr;

    for (size_t i = 0; i < FD_MAX_RETRY_NUM; i++) {
        msr = fd_read_main_status_register();
        if ((msr.bit_expr & (1 << drive_number)) == 0) {
            return AXEL_SUCCESS;
        }
    }

    return AXEL_FAILED;
}


static Axel_state_code fd_write_command(Fdd_constants cmd) {
    for (size_t i = 0; i < FD_MAX_RETRY_NUM; i++) {
        Fd_main_status_register msr = fd_read_main_status_register();
        if (msr.rqm == 1 && msr.dio == 0) {
            io_out8(DATA_FIFO_REGISTER, cmd & 0xFF);
            return AXEL_SUCCESS;
        }
    }

    return AXEL_FAILED;
}


static Axel_state_code fd_read_result(uint8_t* result) {
    for (size_t i = 0; i < FD_MAX_RETRY_NUM; i++) {
        Fd_main_status_register msr = fd_read_main_status_register();
        if ((msr.rqm == 1) && ((msr.dio == 1) || (msr.non_dma == 1))) {
            *result = io_in8(DATA_FIFO_REGISTER);
            return AXEL_SUCCESS;
        }
    }

    return AXEL_FAILED;
}


static void fd_wait_irq(void) {
    while (interrupt_fdd_iqr == false);
}


static Axel_state_code fd_cmd_sense_interrupt(Fd_status_register_st0* st0, uint8_t* present_cylinder_number) {
    if ((st0 == NULL) && (present_cylinder_number == NULL)) {
        return AXEL_FAILED;
    }

    FAILED_RETURN(fd_write_command(SENSE_INTERRUPT));
    FAILED_RETURN(fd_read_result(&st0->bit_expr));
    FAILED_RETURN(fd_read_result(present_cylinder_number));

    return AXEL_SUCCESS;
}


static Axel_state_code fd_cmd_configure(void) {
    FAILED_RETURN(fd_write_command(CONFIGURE));
    /* First byte must be all zero. */
    FAILED_RETURN(fd_write_command(0x00));
    /* Bit 0 - 3: FIFO threshold value. */
    /* Bit 4    : Disable polling. */
    /* Bit 5    : Disable FIFO. */
    /* Bit 6    : Disable implied seek. */
    /* Bit 7    : Reserved by 0. */
    FAILED_RETURN(fd_write_command(0x41));
    /* Bit 0 - 7: pre complement value. */
    FAILED_RETURN(fd_write_command(0x00));

    return AXEL_SUCCESS;
}


static Axel_state_code fd_cmd_specify(bool is_set_dma) {
    FAILED_RETURN(fd_write_command(SPECIFY));

    /* Assumed 3.5 inch floppy drive. */

    /* Set step rate and head unload time. */
    uint8_t parameter = 0xD2;
    FAILED_RETURN(fd_write_command(parameter));

    /* Set head load time and Non-DMA. */
    parameter = 0x20 + ((is_set_dma == true) ? (0) : (1));
    FAILED_RETURN(fd_write_command(parameter));

    return AXEL_SUCCESS;
}


static Axel_state_code fd_cmd_recalibrate(Fd_dev* fdd) {
    fd_set_motor(fdd, true);

    for (size_t i = 0; i < FD_MAX_RETRY_NUM; i++) {
        if (AXEL_SUCCESS != fd_write_command(RECALIBRATE)) {
            continue;
        }
        if (AXEL_SUCCESS != fd_write_command(fdd->drive_number & 0x03)) {
            continue;
        }

        wait(3000);
        fd_wait_irq();

        Fd_status_register_st0 st0;
        uint8_t present_cylinder_number;
        if ((AXEL_SUCCESS != fd_cmd_sense_interrupt(&st0, &present_cylinder_number)) || (st0.interrupt_code != 0)) {
            continue;
        }

        if (present_cylinder_number == 0) {
            /* If head moves to 0, break */
            return AXEL_SUCCESS;
        }
    }

    fd_set_motor(fdd, false);

    return AXEL_FAILED;
}


static Axel_state_code fd_cmd_version(Fd_dev* fdd, uint8_t* v) {
    FAILED_RETURN(fd_write_command(VERSION));
    FAILED_RETURN(fd_read_result(v));

    return AXEL_SUCCESS;
}


static Axel_state_code fd_cmd_seek(Fd_dev* fdd, uint8_t head, uint8_t cylinder) {
    if (fdd->enable_motor == false) {
        fd_set_motor(fdd, true);
    }

    for (size_t i = 0; i < FD_MAX_RETRY_NUM; i++) {
        if (fd_write_command(SEEK) != AXEL_SUCCESS) {
            continue;
        }

        if (fd_write_command((uint8_t)(head << 2) | fdd->drive_number) != AXEL_SUCCESS) {
            continue;
        }

        if (fd_write_command(cylinder) != AXEL_SUCCESS) {
            continue;
        }

        wait(3000);
        fd_wait_irq();

        Fd_status_register_st0 st0;
        uint8_t present_cylinder_number;
        if ((fd_cmd_sense_interrupt(&st0, &present_cylinder_number) != AXEL_SUCCESS) || (st0.interrupt_code != 0x00)) {
            continue;
        }

        if (present_cylinder_number == cylinder) {
            return AXEL_SUCCESS;
        }
    }

    return AXEL_FAILED;
}


static Axel_state_code fd_cmd_read_data(Fd_dev* fdd, uint8_t head, uint8_t cylinder, uint8_t sector, uint8_t* buffer) {
    for (size_t i = 0; i < FD_MAX_RETRY_NUM; i++) {
        /* 0x40 means MFM bit. */
        if (AXEL_SUCCESS != fd_write_command(0x40 | READ_DATA))                         { continue; }
        if (AXEL_SUCCESS != fd_write_command((uint8_t)(head << 2) | fdd->drive_number)) { continue; }
        if (AXEL_SUCCESS != fd_write_command(cylinder))                                 { continue; }
        if (AXEL_SUCCESS != fd_write_command(head))                                     { continue; }
        if (AXEL_SUCCESS != fd_write_command(sector))                                   { continue; }
        if (AXEL_SUCCESS != fd_write_command(2))                                        { continue; }
        if (AXEL_SUCCESS != fd_write_command(FD_SECTOR_PER_TRACK))                      { continue; }
        if (AXEL_SUCCESS != fd_write_command(0x1B))                                     { continue; }
        if (AXEL_SUCCESS != fd_write_command(0xFF))                                     { continue; }

        break;
    }

    fd_wait_irq();

    /* TODO */

    uint8_t result_statuses[7];
    for (size_t i = 0; i < ARRAY_SIZE_OF(result_statuses); i++) {
        FAILED_RETURN(fd_read_result(&result_statuses[i]));
    }

    return AXEL_SUCCESS;
}


/*
 * The BIOS probably leaves the controller in its default state.
 *  Drive polling mode on
 *  FIFO off
 *  threshold = 1
 *  implied seek off
 *  lock off
 */
static Axel_state_code fd_reset_controller(Fd_dev* fdd) {
    Fd_digital_output_register dor = { .bit_expr = 0, };
    dor.drive_selector = fdd->drive_number & 0xF;

    /* Reset controller. */
    dor.not_reset = 0;
    io_out8(DIGITAL_OUTPUT_REGISTER, dor.bit_expr);

    wait(10);

    /* Enable controller. */
    dor.not_reset      = 1;
    dor.enable_iqr_dma = 1;
    io_out8(DIGITAL_OUTPUT_REGISTER, dor.bit_expr);

    fd_wait_irq();

    Fd_status_register_st0 st0;
    uint8_t present_cylinder_number;
    int i;
    for (i = 0; i < 4; i++) {
        int tmp = i;
        if (AXEL_FAILED == fd_cmd_sense_interrupt(&st0, &present_cylinder_number)) {
            i = tmp - 1;
        }
    }

    /* Reset command erases some settings */
    /* Set time configuration. */
    if (AXEL_SUCCESS != fd_cmd_specify(false)) {
        return AXEL_FAILED;
    }

    /* Set data transfer rate. */
    io_out8(CONFIGURATION_CONTROL_REGISTER, 0x00);

    /* Calibrate a head position. */
    return fd_cmd_recalibrate(fdd);
}


static Axel_state_code fd_init(Fd_dev* fdd) {
    fdd->drive_number = FD_DRIVE0;

    uint8_t ver = 0;
    FAILED_RETURN(fd_cmd_version(fdd, &ver));
    if (ver != 0x90) {
        /* Not supported FDD. */
        return AXEL_FAILED;
    }

    enable_pic_port(PIC_IMR_MASK_IRQ06);

    fd_set_motor(fdd, false);

    if (fd_cmd_configure() == AXEL_FAILED) {
        return AXEL_FAILED;
    }

    if (fd_reset_controller(fdd) == AXEL_FAILED) {
        return AXEL_FAILED;
    }
    fdd->is_initialize = true;

    return AXEL_SUCCESS;
}


static void lba_to_chs(uint32_t lba, uint8_t* head, uint8_t* cylinder, uint8_t* sector) {
    *head     = (uint8_t)(lba / (FD_HEAD * FD_SECTOR_PER_TRACK));
    *cylinder = (lba / FD_SECTOR_PER_TRACK ) % FD_HEAD;
    *sector   = (lba % FD_SECTOR_PER_TRACK) + 1;
}


static Axel_state_code fd_block_access(void* p, uint8_t direction, uint32_t lba, uint8_t sec_cnt, uint8_t* buffer) {
    Fd_dev* fdd = p;

    if (fdd->enable_motor == false) {
        fd_set_motor(fdd, true);
    }

    uint8_t head;
    uint8_t cylinder;
    uint8_t sector;
    lba_to_chs(lba, &head, &cylinder, &sector);

    FAILED_RETURN(fd_cmd_seek(fdd, head, cylinder));
    /* FAILED_RETURN(fd_cmd_read_data(fdd, head, cylinder, sector, buffer)); */
    if (fd_cmd_read_data(fdd, head, cylinder, sector, buffer) == AXEL_FAILED) {
        fill_screen(0x1);
        INF_LOOP();
    }

    return AXEL_SUCCESS;
}


void interrupt_fdd(void) {
    interrupt_fdd_iqr = true;
    send_done_pic_master();
}


extern void loader2_interrupt_timer(void);
extern void loader2_interrupt_fdd(void);
static void set_idt(Gate_descriptor* idts, size_t size) {
    set_gate_descriptor(idts + 0x20, (uintptr_t)loader2_interrupt_timer, KERNEL_CODE_SEGMENT_INDEX, GD_FLAGS_INT);
    set_gate_descriptor(idts + 0x26, (uintptr_t)loader2_interrupt_fdd, KERNEL_CODE_SEGMENT_INDEX, GD_FLAGS_INT);
    interrupt_fdd_iqr = false;
}


static _Noreturn void no_op(void) {
    uint8_t color = 1;
    for (;;) {
        fill_screen(color++);
    }
}


static inline void clear_bss(void) {
    extern uintptr_t const LD_LOADER2_BSS_BEGIN;
    extern uintptr_t const LD_LOADER2_BSS_END;

    for (uintptr_t i = LD_LOADER2_BSS_BEGIN; i < LD_LOADER2_BSS_END; i++) {
        *((uint8_t*)i) = 0;
    }
}


static inline void fill_screen(uint8_t color) {
    for (uint8_t* v = (uint8_t*)0xA0000; (uintptr_t)v < 0xB0000; v++) {
        *v = color;
    }
}
