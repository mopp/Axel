/**
 * @file include/segment.h
 * @brief segment descriptor header.
 * @author mopp
 * @version 0.1
 * @date 2014-07-22
 */


#ifndef _SEGMENT_H_
#define _SEGMENT_H_



#define NULL_SEGMENT_INDEX        (0)
#define KERNEL_CODE_SEGMENT_INDEX (1)
#define KERNEL_DATA_SEGMENT_INDEX (2)
#define USER_CODE_SEGMENT_INDEX   (3)
#define USER_DATA_SEGMENT_INDEX   (4)
#define KERNEL_TSS_SEGMENT_INDEX  (5)

#define SEGMENT_NUM               (5 + 1) /* There are "+1" to allocate null descriptor. */

#define KERNEL_PRIVILEGE_LEVEL 0
#define USER_PRIVILEGE_LEVEL   3

/* These is set into CS, DS and etc. */
#define KERNEL_CODE_SEGMENT_SELECTOR ((8 * KERNEL_CODE_SEGMENT_INDEX) + KERNEL_PRIVILEGE_LEVEL)
#define KERNEL_DATA_SEGMENT_SELECTOR ((8 * KERNEL_DATA_SEGMENT_INDEX) + KERNEL_PRIVILEGE_LEVEL)
#define USER_CODE_SEGMENT_SELECTOR   ((8 * USER_CODE_SEGMENT_INDEX) + USER_PRIVILEGE_LEVEL)
#define USER_DATA_SEGMENT_SELECTOR   ((8 * USER_DATA_SEGMENT_INDEX) + USER_PRIVILEGE_LEVEL)
#define KERNEL_TSS_SEGMENT_SELECTOR  ((8 * KERNEL_TSS_SEGMENT_INDEX) + KERNEL_PRIVILEGE_LEVEL)



#ifndef _ASSEMBLY_H_



/*
 * Global Segment Descriptor
 * Segment limit high + low is 20 bit.
 * Segment base address low + mid + high is 32 bit.
 */
union segment_descriptor {
    struct {
        uint16_t limit_low;               /* Segment limit low. */
        uint16_t base_addr_low;           /* Segment address low. */
        uint8_t base_addr_mid;            /* Segment address mid. */
        uint8_t type : 4;                 /* This shows segment main configuration. */
        unsigned int segment_type : 1;    /* If 0, segment is system segment, if 1, segment is code or data segment. */
        unsigned int plivilege_level : 2; /* This controles accesse level. */
        unsigned int present : 1;         /* Is it exist on memory */
        unsigned int limit_hi : 4;        /* Segment limit high. */
        unsigned int available : 1;       /* OS can use this.  */
        unsigned int zero_reserved : 1;   /* This keeps 0. */
        unsigned int op_size : 1;         /* If 0, 16bit segment, If 1 32 bit segment. */
        unsigned int granularity : 1;     /* If 0 unit is 1Byte, If 1 unit is 4KB */
        uint8_t base_addr_hi;             /* Segment address high. */
    };
    struct {
        uint32_t bit_expr_low;
        uint32_t bit_expr_high;
    };
};
typedef union segment_descriptor Segment_descriptor;
_Static_assert(sizeof(Segment_descriptor) == 8, "Static ERROR : Segment_descriptor size is NOT 8 byte(64 bit).");


enum GDT_constants {
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
    GDT_FLAG_TYPE_TSS          = 0x000900,
    GDT_FLAG_TSS_BUSY          = 0x000200,
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
    GDT_FLAGS_TSS              = GDT_FLAG_TYPE_TSS | GDT_FLAG_RING0 | GDT_FLAG_PRESENT | GDT_FLAG_GRANULARIT,
};


struct task_state_segment {
    uint16_t back_link;
    uint16_t reserved0;
    uint32_t esp0;           /* Stack pointer register value of privileged level 0 */
    uint16_t ss0, reserved1; /* Stack segment register value of privileged level 0*/
    uint32_t esp1;           /* Stack pointer register value of privileged level 1 */
    uint16_t ss1, reserved2; /* Stack segment register value of privileged level 1*/
    uint32_t esp2;           /* Stack pointer register value of privileged level 2 */
    uint16_t ss2, reserved3; /* Stack segment register value of privileged level 2*/
    uint32_t cr3;            /* Page directly base register value. */
    uint32_t eip;            /* eip register value before task switching. */
    uint32_t eflags;         /* eflags register value before task switching. */
    uint32_t eax, ecx, edx, ebx;
    uint32_t esp, ebp, esi, edi;
    uint16_t es, reserved4;
    uint16_t cs, reserved5;
    uint16_t ss, reserved6;
    uint16_t ds, reserved7;
    uint16_t fs, reserved8;
    uint16_t gs, reserved9;
    uint16_t ldtr, reserved10;
    uint16_t debug_trap_flag : 1;
    uint16_t reserved11 : 15;
    uint16_t iomap_base_addr;
};
typedef struct task_state_segment Task_state_segment;
_Static_assert(sizeof(Task_state_segment) == 104, "Task_state_segment size is NOT 104 byte");


struct segment_selector {
    uint16_t request_privilege_level : 2;
    uint16_t table_indicator : 1;
    uint16_t index : 13;
};
typedef struct segment_selector Segment_selector;
_Static_assert(sizeof(Segment_selector) == 2, "Segment_selector size is NOT 16bit");



#endif  /* _ASSEMBLY_H_ */



#endif
