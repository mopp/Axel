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
    ACPI_VERSION_1           = 0,
    ACPI_VERSION_2           = 1,
    ACPI_VERSION_3           = 2,
    ACPI_SCI_EN              = 0x1,
    ACPI_SLP_EN              = PO2(13),
    GEN_ADDR_SPACE_SYS_MEM   = 0,
    GEN_ADDR_SPACE_SYS_IO    = 1,
    GEN_ADDR_SPACE_PIC_CONF  = 2,
    GEN_ADDR_SPACE_EMBD_CTRL = 3,
    GEN_ADDR_SPACE_SMBUS     = 4,
    GEN_ADDR_SPACE_FFH       = 0x7f,
    GEN_ACCESS_SIZE_BYTE     = 1,
    GEN_ACCESS_SIZE_WORD     = 2,
    GEN_ACCESS_SIZE_DWORD    = 3,
    GEN_ACCESS_SIZE_QWORD    = 4,
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
static uintptr_t alloc_vaddr_fadt;
static size_t alloc_vnr_fadt;
static Fadt* fadt_g;
static uint16_t acpi_slp_typa;
static uint16_t acpi_slp_typb;


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


static inline void* find_entry(uintptr_t* candidates, size_t nr, char const* sig, size_t signr) {
    static size_t const fadt_frame_nr = 2;
    size_t vaddr_idx = 0;

    /* alloc some pages for sdt */
    alloc_vnr_fadt = fadt_frame_nr * nr;
    alloc_vaddr_fadt = get_free_vmems(alloc_vnr_fadt);

    Sdt_header* sdt = NULL;

    for (size_t i = 0; i < nr; i++) {
        uintptr_t const addr = candidates[i];
        uintptr_t const paddr = (addr >> PTE_FRAME_ADDR_SHIFT_NUM) << PTE_FRAME_ADDR_SHIFT_NUM;
        uintptr_t const b = vaddr_idx * FRAME_SIZE;
        uintptr_t const e = ((vaddr_idx + fadt_frame_nr) * FRAME_SIZE);
        map_page_area(axel_s.kernel_pdt, PDE_FLAGS_KERNEL_DYNAMIC, PTE_FLAGS_KERNEL_DYNAMIC, alloc_vaddr_fadt + b, alloc_vaddr_fadt + e, paddr, paddr + (FRAME_SIZE * fadt_frame_nr));

        uintptr_t sdt_addr = (alloc_vaddr_fadt + b) + (addr - paddr);
        sdt = (Sdt_header*)sdt_addr;
        if ((memcmp(sdt->signature, sig, signr) == 0) && validate_sdt(sdt) == true) {
            /* If detect fadt, copy into buffer. */
            break;
        }

        vaddr_idx += fadt_frame_nr;
    }

    return sdt;
}


static inline Axel_state_code decode_dsdt(Sdt_header* dsdt) {
    uintptr_t len;
    uint8_t* addr;
    uint8_t sz;

    len = dsdt->length - sizeof(Sdt_header);
    addr = (uint8_t*)((uintptr_t)dsdt + sizeof(Sdt_header));

    /* Search \_S5 package in the DSDT */
    while (len >= 5) {
        if (memcmp(addr, "_S5_", 4) == 0) {
            break;
        }
        addr++;
        len--;
    }

    if (len >= 5) {
        /* S5 was found */
        if ((0x12 != *(addr + 4)) || (0x08 != *(addr - 1) && !(0x08 == *(addr - 2) && '\\' == *(addr - 1)))) {
            /* Invalid AML (0x12 = PackageOp) */
            /* Invalid AML (0x08 = NameOp) */
            return AXEL_FAILED;
        }

        /* Skip _S5_\x12 */
        addr += 5;
        len -= 5;

        /* Calculate the size of the packet length */
        sz = (*addr & 0xc0) >> 6;

        /* Check the length and skip packet length and the number of elements */
        if (len < sz + 2) {
            return AXEL_FAILED;
        }

        addr += sz + 2;
        len -= sz + 2;

        /* SLP_TYPa */
        if (0x0a == *addr) {
            /* byte prefix */
            if (len < 1) {
                return AXEL_FAILED;
            }
            addr++;
            len--;
        }
        if (len < 1) {
            return AXEL_FAILED;
        }
        acpi_slp_typa = ((*addr) << 10) & 0xffff;
        addr++;
        len--;

        /* SLP_TYPb */
        if (0x0a == *addr) {
            /* byte prefix */
            if (len < 1) {
                return AXEL_FAILED;
            }
            addr++;
            len--;
        }
        if (len < 1) {
            return AXEL_FAILED;
        }
        acpi_slp_typb = ((*addr) << 10) & 0xffff;
        addr++;
        len--;

        return AXEL_SUCCESS;
    }

    return AXEL_FAILED;
}


Axel_state_code enable_acpi(void) {
    if (fadt_g == NULL) {
        return AXEL_FAILED;
    }

    if ((io_in16(ECAST_UINT16(fadt_g->pm1a_ctrl_block)) & ACPI_SCI_EN) == 0) {

        /* send */
        io_out8(ECAST_UINT16(fadt_g->smi_cmd_port), fadt_g->acpi_enable);

        if ((io_in16(ECAST_UINT16(fadt_g->pm1a_ctrl_block)) & ACPI_SCI_EN) == 0) {
            printf("cannot enable ACPI.\n");
            return AXEL_FAILED;
        }
    }

    return AXEL_SUCCESS;
}


Axel_state_code shutdown(void) {
    if (enable_acpi() != AXEL_SUCCESS) {
        return AXEL_FAILED;
    }

    io_out16(ECAST_UINT16(fadt_g->pm1a_ctrl_block), acpi_slp_typa | ACPI_SLP_EN);
    io_out16(ECAST_UINT16(fadt_g->pm1b_ctrl_block), acpi_slp_typb | ACPI_SLP_EN);

    return AXEL_SUCCESS;
}


Axel_state_code init_acpi(void) {
    Rsdp* rsdp = NULL;

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

    /* Copy table address. */
    size_t table_nr = (rsdt->h.length - sizeof(Sdt_header)) / sizeof(uintptr_t);
    uintptr_t* table_addrs = kmalloc(sizeof(uintptr_t) * table_nr);
    for (size_t i = 0; i < table_nr; i++) {
        table_addrs[i] = (uintptr_t)rsdt->tables[i];
    }

    fadt_g = find_entry(table_addrs, table_nr, fadt_signature, 4);
    if (fadt_g == NULL) {
        return AXEL_FAILED;
    }

    uintptr_t v = get_free_vmems(1);
    paddr = (fadt_g->dsdt_addr >> PTE_FRAME_ADDR_SHIFT_NUM) << PTE_FRAME_ADDR_SHIFT_NUM;
    map_page(axel_s.kernel_pdt, PDE_FLAGS_KERNEL_DYNAMIC, PTE_FLAGS_KERNEL_DYNAMIC, v, paddr);
    Sdt_header* dsdt = (Sdt_header*)(v + (fadt_g->dsdt_addr - paddr));

    if (memcmp(dsdt->signature, "DSDT", 4) != 0) {
        return AXEL_FAILED;
    }

    if (decode_dsdt(dsdt) != AXEL_SUCCESS) {
        Sdt_header* ssdt = find_entry(table_addrs, table_nr, "SSDT", 4);
        if (memcmp(ssdt->signature, "SSDT", 4) != 0) {
            return AXEL_FAILED;
        }
    }

    unmap_page(v);
    unmap_page(vaddr_rsdt);
    kfree(table_addrs);

    return AXEL_SUCCESS;
}
