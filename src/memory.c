/**
 * @file memory.c
 * @brief physical memory initialize and manage.
 *      You MUST NOT use get_new_dlinked_list_node().
 *      Bacause get_new_dlinked_list_node() include malloc.
 *      So, You MUST use list_get_new_memory_node().
 * @author mopp
 * @version 0.1
 * @date 2014-06-05
 */


#include <memory.h>
#include <paging.h>
#include <asm_functions.h>
#include <utils.h>
#include <buddy.h>
#include <macros.h>
#include <kernel.h>


/*
 * LD_* variables are set by linker. see kernel.ld
 * In kernel.ld, kernel start address is defined (LD_KERNEL_PHYSICAL_BASE_ADDR + LD_KERNEL_VIRTUAL_BASE_ADDR)
 * But, I want to make mapping start physical address 0x00000000.
 * So, I will subtract LD_KERNEL_PHYSICAL_BASE_ADDR from LD_KERNEL_START.
 * And Multiboot_* struct is positioned at virtual address 0xC000000~0xC0010000.
 */
extern uintptr_t const LD_KERNEL_START;
extern uintptr_t const LD_KERNEL_END;
extern uintptr_t const LD_KERNEL_SIZE;
static uintptr_t const kernel_start_addr = (uintptr_t)&LD_KERNEL_START - KERNEL_PHYSICAL_BASE_ADDR;
static uintptr_t kernel_end_addr = (uintptr_t)&LD_KERNEL_END;

static size_t total_memory_size;

static void fix_address(Multiboot_info* const);
static uintptr_t align_address(uintptr_t, size_t);
static void* smalloc(size_t);
static void* smalloc_align(size_t, size_t);


Axel_state_code init_memory(Multiboot_info* const mb_info) {
    /*
     * pointer of Multiboot_info struct has physical address.
     * So, this function converts physical address to virtual address to avoid pagefault.
     */
    fix_address(mb_info);

    if (mb_info->flags.is_mem_enable == 0) {
        return AXEL_ERROR_INITIALIZE_MEMORY;
    }

    /*
     * Set physical 0x000000 to "physical kernel end address" is allocated.
     * But, there are continuous allocated areas in this area.
     * So, We alloc it to one node.
     * NOTE from http://wiki.osdev.org/Detecting_Memory_(x86)#Memory_Detection_in_Emulators
     *      When you tell an emulator how much memory you want emulated,
     *      the concept is a little "fuzzy" because of the emulated missing bits of RAM below 1M.
     *      If you tell an emulator to emulate 32M, does that mean your address space definitely goes from 0 to 32M -1, with missing bits? Not necessarily.
     *      The emulator might assume that you mean 32M of contiguous memory above 1M, so it might end at 33M -1.
     *      Or it might assume that you mean 32M of total usable RAM, going from 0 to 32M + 384K -1.
     *      So don't be surprised if you see a "detected memory size that does not exactly match your expectations".
     * qemu
     *       size addr begin        end                  size       type
     * 0x00000014 0x00000000 0x0009fc00 0x0009fc00 (639   KB) 0x00000001 boot_info->mem_lower
     * 0x00000014 0x0009fc00 0x000a0000 0x00000400 (1     KB) 0x00000002
     *            0x000a0000 0x000f0000 0x00050000 (320   KB) hole - BIOS and vram
     * 0x00000014 0x000f0000 0x00100000 0x00010000 (64    KB) 0x00000002
     * 0x00000014 0x00100000 0x01fe0000 0x01ee0000 (31616 KB) 0x00000001 boot_info->mem_upper
     * 0x00000014 0x01fe0000 0x02000000 0x00020000 (128   KB) 0x00000002
     * 0x00000014 0xfffc0000 0xffff0000 0x00040000 (256   KB) 0x00000002
     * Total size: 0x01ff0000 = 32704 KB = 31 MB
     * 32704 + 320 = 33024 KB
     * 0xfd000000 - vbe
     */
    Multiboot_memory_map const* mmap = (Multiboot_memory_map const*)((uintptr_t)mb_info->mmap_addr);
    Multiboot_memory_map const* const limit = (Multiboot_memory_map const* const)((uintptr_t)mmap + mb_info->mmap_length);
    Multiboot_memory_map free_area = {.len = 0};
    total_memory_size = 0;
    while (mmap < limit) {
        total_memory_size += mmap->len;

        if ((mmap->type == MULTIBOOT_MEMORY_AVAILABLE) && (get_kernel_phys_start_addr() < mmap->addr) && (free_area.len <= mmap->len)) {
            /* Select memory space for buddy. */
            free_area = *mmap;
        }

        mmap = (Multiboot_memory_map*)((uintptr_t)mmap + sizeof(mmap->size) + mmap->size);
    }

    if (free_area.len == 0) {
        return AXEL_ERROR_INITIALIZE_MEMORY;
    }

    /*
     * Allocation of kernel variables area.
     * These variables must be necessarily.
     * But it is too big to allocate in a compile time.
     * 4150 KB.
     */
    axel_s.bman       = smalloc(sizeof(Buddy_manager));
    axel_s.tman       = smalloc(sizeof(Tlsf_manager));
    axel_s.kernel_pdt = smalloc_align(ALL_PAGE_STRUCT_SIZE, PAGE_SIZE);

    /*
     * Calculate the number of frame for buddy system.
     * And allocate those frames.
     */
    uintptr_t end_addr = (free_area.addr + free_area.len) & 0xffffffff;
    uintptr_t addr_len = end_addr - get_kernel_phys_end_addr();
    size_t frame_nr    = addr_len / FRAME_SIZE;
    Frame* frames      = smalloc(sizeof(Frame) * frame_nr);

    kernel_end_addr = align_address(kernel_end_addr, 4096);

    /*
     * Re calculate the number of frame.
     * Because of avoiding buddy system error.
     * In this, frame_nr is smaller than before.
     */
    addr_len = end_addr - get_kernel_phys_end_addr();
    frame_nr = addr_len / FRAME_SIZE;

    axel_s.bman = buddy_init(axel_s.bman, get_kernel_phys_end_addr(), frames, frame_nr);

    if (frames == NULL || axel_s.bman == NULL) {
        return AXEL_ERROR_INITIALIZE_MEMORY;
    }

    /*
     * Physical memory managing is just finished.
     * Next, let's set paging.
     */
    init_paging();

    return AXEL_SUCCESS;
}


/**
 * @brief Physical memory allocation.
 * @param size allocated memory size.
 * @return pointer to allocated memory.
 */
void* pmalloc(size_t size) {
    Frame* f = buddy_alloc_frames(axel_s.bman, size_to_order(size));

    return (void*)(get_frame_addr(axel_s.bman, f));
}


void pfree(void* obj) {
    buddy_free_frames(axel_s.bman, get_frame_by_addr(axel_s.bman, (uintptr_t)obj));
}


uintptr_t get_kernel_vir_start_addr(void) {
    return kernel_start_addr;
}


uintptr_t get_kernel_vir_end_addr(void) {
    return kernel_end_addr;
}


uintptr_t get_kernel_phys_start_addr(void) {
    return vir_to_phys_addr(kernel_start_addr);
}


uintptr_t get_kernel_phys_end_addr(void) {
    return vir_to_phys_addr(kernel_end_addr);
}


size_t get_kernel_size(void) {
    return kernel_end_addr - kernel_start_addr;
}


size_t get_kernel_static_size(void) {
    return (uintptr_t)&LD_KERNEL_SIZE;
}


size_t get_total_memory_size(void) {
    /*
     * smalloc expands kernel_end_addr to allocate early memory.
     * And buddy system manages area after kernel_end_addr.
     */
    return get_kernel_phys_end_addr() + buddy_get_total_memory_size(axel_s.bman);
}


/**
 * @brief Strangely memory allocation.
 *        This function allocate contiguous memory at kernel tail.
 * @param size allocated memory size.
 * @return pointer to allocated memory.
 */
static inline void* smalloc(size_t size) {
    uintptr_t addr = kernel_end_addr;

    /* size add to kernel_end_addr to allocate memory. */
    kernel_end_addr += size;

    return (void*)addr;
}


static inline uintptr_t align_address(uintptr_t addr, size_t align_size) {
    size_t const a = align_size - 1;
    return ((addr + a) & ~(a));
}


/**
 * @brief alignment smalloc.
 *        return address is alignment by "align_size"
 * @param size       allocated memory size.
 * @param align_size alignment size that is only power of 2.
 * @return pointer to allocated memory.
 */
static inline void* smalloc_align(size_t size, size_t align_size) {
    kernel_end_addr = align_address(kernel_end_addr, align_size);

    return smalloc(size);
}


static inline void fix_address(Multiboot_info* const mb_info) {
    set_phys_to_vir_addr(&mb_info->cmdline);
    set_phys_to_vir_addr(&mb_info->mods_addr);
    set_phys_to_vir_addr(&mb_info->mmap_addr);
    set_phys_to_vir_addr(&mb_info->drives_addr);
    set_phys_to_vir_addr(&mb_info->config_table);
    set_phys_to_vir_addr(&mb_info->boot_loader_name);
    set_phys_to_vir_addr(&mb_info->apm_table);
    set_phys_to_vir_addr(&mb_info->vbe_control_info);
    set_phys_to_vir_addr(&mb_info->vbe_mode_info);
}

