/**
 * @file acpi.c
 * @brief Advanced Configuration and Power Interface implementation.
 * @author mopp
 * @version 0.1
 * @date 2014-10-15
 */

#include <stdint.h>
#include <utils.h>
#include <paging.h>
#include <memory.h>
#include <kernel.h>
#include <macros.h>
#include <asm_functions.h>


enum {
    ACPI_VERSION_1 = 0,
    ACPI_VERSION_2 = 1,
    ACPI_VERSION_3 = 2,
    GEN_ADDR_SPACE_SYS_MEM = 0,
    GEN_ADDR_SPACE_SYS_IO = 1,
    GEN_ADDR_SPACE_PIC_CONF = 2,
    GEN_ADDR_SPACE_EMBD_CTRL = 3,
    GEN_ADDR_SPACE_SMBUS = 4,
    GEN_ADDR_SPACE_FFH = 0x7f,
    GEN_ACCESS_SIZE_BYTE = 1,
    GEN_ACCESS_SIZE_WORD = 2,
    GEN_ACCESS_SIZE_DWORD = 3,
    GEN_ACCESS_SIZE_QWORD = 4,
};


struct generic_address {
    uint8_t addr_space;
    uint8_t bit_width;
    uint8_t bit_offset;
    uint8_t access_size;
    uint64_t addr;
};
typedef struct generic_address Generic_address;


/* Root System Description Pointer. */
struct rsdp {
    uint8_t signature[8];
    uint8_t checksum;
    uint8_t oem_id[6];
    uint8_t revision;
    uint32_t rsdt_addr;
    /* if version 2.0 (revision value is 1), available below. */
    uint32_t length;
    uint64_t xsdt_addr;
    uint8_t extended_checksum;
    uint8_t reserved[3];
};
typedef struct rsdp Rsdp;


/* System Description Table Header. */
struct sdt_header {
    uint8_t signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    uint8_t oem_id[6];
    uint8_t oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
};
typedef struct sdt_header Sdt_header;


/* Root System Description Table. */
struct rsdt {
    Sdt_header h;
    uint32_t tables[];
};
typedef struct rsdt Rsdt;


/* Fixed ACPI Description Table */
struct fadt {
    Sdt_header h;
    uint32_t firmware_ctrl;
    uint32_t dsdt_addr;
    uint8_t reserved;
    uint8_t preferred_pm_prof;
    uint16_t sci_interrupt;
    uint32_t smi_cmd_port;
    uint8_t acpi_enable;
    uint8_t acpi_disable;
    uint8_t s4bios_req;
    uint8_t pstate_ctrl;
    uint32_t pm1a_event_block;
    uint32_t pm1b_event_block;
    uint32_t pm1a_ctrl_block;
    uint32_t pm1b_ctrl_block;
    uint32_t pm2_ctrl_block;
    uint32_t pm_timer_block;
    uint32_t gpe0_block;
    uint32_t gpe1_block;
    uint8_t pm1_event_length;
    uint8_t pm1_ctrl_length;
    uint8_t pm2_ctrl_length;
    uint8_t pm_timer_length;
    uint8_t gpe0_length;
    uint8_t gpe1_length;
    uint8_t gpe1_base;
    uint8_t cstate_ctrl;
    uint16_t worstc2_latency;
    uint16_t worstc3_latency;
    uint16_t flush_size;
    uint16_t flush_stride;
    uint8_t duty_offset;
    uint8_t duty_Width;
    uint8_t day_alarm;
    uint8_t month_alarm;
    uint8_t century;
    uint16_t boot_architecture_flags;
    uint8_t reserved2;
    uint32_t flags;
    Generic_address reset_reg;
    uint8_t reset_value;
    uint8_t reserved3[3];
    uint64_t x_firmware_ctrl;
    uint64_t x_dsdt;
    Generic_address x_pm1a_event_block;
    Generic_address x_pm1b_event_block;
    Generic_address x_pm1a_ctrl_block;
    Generic_address x_pm1b_ctrl_block;
    Generic_address x_pm2_controlblock;
    Generic_address x_pm_timer_block;
    Generic_address x_gpe0_block;
    Generic_address x_gpe1_block;
};
typedef struct fadt Fadt;


static char const* rsdp_signature = "RSD PTR ";
static char const* fadt_signature = "FACP";


static inline bool validate_rsdp(Rsdp* rsdp) {
    uint32_t sum = 0;
    switch (rsdp->revision) {
        case ACPI_VERSION_1:
            for (int i = 0; i < offsetof(Rsdp, length); i++) {
                sum += ((uint8_t*)rsdp)[i];
            }
            break;
        case ACPI_VERSION_2:
            for (int i = 0; i < sizeof(Rsdp); i++) {
                sum += ((uint8_t*)rsdp)[i];
            }
            break;
    }

    return ((sum & 0xff) == 0) ? true : false;
}


static inline bool validate_sdt(Sdt_header* sdt) {
    if (sdt->length == 0) {
        return false;
    }

    uint32_t sum = 0;
    for (size_t i = 0; i < sdt->length; i++) {
        sum += ((uint8_t*)sdt)[i];
    }

    return ((sum & 0xff) == 0) ? true : false;
}


static inline Rsdp* find_rsdp(uintptr_t begin, uintptr_t end) {
    do {
        if ((memcmp((void*)begin, rsdp_signature, 8) == 0) && (validate_rsdp((Rsdp*)begin) == true)) {
            return (Rsdp*)begin;
        }
        begin += 0x10;
    } while (begin < end);

    return NULL;
}


static uintptr_t alloc_vmem_addr;
static size_t alloc_vmem_frame_nr;
Axel_state_code init_acpi(void) {
    Rsdp* rsdp;
    /*
     * Search the main BIOS area below 1 MB
     * This signature is always on a 16 byte boundary.
     */
    uintptr_t b = phys_to_vir_addr(0x000E0000);
    uintptr_t e = phys_to_vir_addr(0x00100000);
    rsdp = find_rsdp(b, e);

    if (rsdp == NULL) {
        /*
         * Search the first 1 KB of the EBDA (Extended BIOS Data Area)
         * A 2 byte real mode segment pointer to it is located at 0x040E,
         */
        uint16_t real_seg_ptr = *((uint16_t*)(phys_to_vir_addr(0x40E)));
        b = phys_to_vir_addr((uintptr_t)(real_seg_ptr << 4));
        e = e + 1024;
        rsdp = find_rsdp(b, e);
    }

    if (rsdp == NULL) {
        return AXEL_FAILED;
    }

    /* Get and map pages for working. */
    uintptr_t const vaddr_rsdt = get_free_vmems(1);
    uintptr_t paddr = (rsdp->rsdt_addr >> PTE_FRAME_ADDR_SHIFT_NUM) << PTE_FRAME_ADDR_SHIFT_NUM;
    map_page(axel_s.kernel_pdt, PDE_FLAGS_KERNEL_DYNAMIC, PTE_FLAGS_KERNEL_DYNAMIC, vaddr_rsdt, paddr);

    Rsdt* rsdt = (Rsdt*)(vaddr_rsdt + (rsdp->rsdt_addr - paddr));
    if (validate_sdt(&rsdt->h) == false) {
        unmap_page(vaddr_rsdt);
        return AXEL_FAILED;
    }
    size_t table_nr = (rsdt->h.length - sizeof(Sdt_header)) / sizeof(uintptr_t);

    /* Copy table address. */
    uintptr_t* table_addrs = kmalloc(sizeof(uintptr_t) * table_nr);
    for (size_t i = 0; i < table_nr; i++) {
        table_addrs[i] = (uintptr_t)rsdt->tables[i];
    }

    /* alloc some pages for sdt */
    size_t fadt_frame_nr = 2;
    alloc_vmem_frame_nr  = fadt_frame_nr * table_nr;
    alloc_vmem_addr      = get_free_vmems(alloc_vmem_frame_nr);
    size_t vaddr_idx     = 0;
    Fadt* fadt = NULL;
    for (size_t i = 0; i < table_nr; i++) {
        uintptr_t const addr  = table_addrs[i];
        uintptr_t const paddr = (addr >> PTE_FRAME_ADDR_SHIFT_NUM) << PTE_FRAME_ADDR_SHIFT_NUM;
        uintptr_t const b     = vaddr_idx * FRAME_SIZE;
        uintptr_t const e     = ((vaddr_idx + fadt_frame_nr) * FRAME_SIZE);
        map_page_area(axel_s.kernel_pdt, PDE_FLAGS_KERNEL_DYNAMIC, PTE_FLAGS_KERNEL_DYNAMIC, alloc_vmem_addr + b, alloc_vmem_addr + e, paddr, paddr + (FRAME_SIZE * fadt_frame_nr));

        uintptr_t sdt_addr = (alloc_vmem_addr + b) + (addr - paddr);
        Sdt_header* sdt = (Sdt_header*)sdt_addr;
        if ((memcmp(sdt->signature, fadt_signature, 4) == 0) && validate_sdt(sdt) == true) {
            /* If detect fadt, copy into buffer. */
            fadt = (Fadt*)sdt_addr;
        }

        vaddr_idx += fadt_frame_nr;
    }
    unmap_page(vaddr_rsdt);
    kfree(table_addrs);

#if 0
    printf("%p\n", fadt);
    printf("command port: 0x%x\n", fadt->smi_cmd_port);
    printf("acpi enable : %d\n", fadt->acpi_enable);
    printf("acpi disable: %d\n", fadt->acpi_disable);
    uint16_t u = io_in16(fadt->pm1a_ctrl_block & 0xff);
    printf("%d\n", u);
    printf("%d\n", u & 1);

    io_out8(fadt->sci_interrupt & 0xffff, fadt->acpi_disable);
    u = io_in16(fadt->pm1a_ctrl_block & 0xff);
    printf("%d\n", u);

    printf("0x%x\n", fadt->sci_interrupt);
#endif

    return AXEL_SUCCESS;
}
