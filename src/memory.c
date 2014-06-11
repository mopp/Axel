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
#include <string.h>
#include <list.h>


enum memory_manager_constants {
    MAX_MEM_NODE_NUM = 2048, /* MAX_MEM_NODE_NUM(0x800) must be a power of 2 for below modulo. */
    MEM_INFO_STATE_FREE = 0, /* this shows that we can use its memory. */
    MEM_INFO_STATE_ALLOC,    /* this shows that we cannot use its memory. */
};
#define MOD_MAX_MEM_NODE_NUM(n) (n & 0x04ffu)


/* memory infomation structure to managed. */
struct memory_info {
    uintptr_t base_addr; /* base address */
    uint8_t state;       /* managed area state */
    size_t size;         /* allocated size */
};
typedef struct memory_info Memory_info;


/* structure for storing memory infomation. */
struct memory_manager {
    List list; /* this list store memory infomation using Memory_info structure. */
};
typedef struct memory_manager Memory_manager;



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

/* These pointer variables is dynamically allocated at kernel tail. */
static Memory_manager* mem_man;
static List_node* mem_list_nodes;
static Memory_info* mem_info;
static bool* used_list_node;

static void fix_address(Multiboot_info* const);
static List_node* list_get_new_memory_node(void);
static void remove_memory_list_node(List_node*);
static void* smalloc(size_t);
static void* smalloc_page_round(size_t);


void init_memory(Multiboot_info* const mb_info) {
    /*
     * pointer of Multiboot_info struct has physical address.
     * So, this function converts physical address to virtual address to avoid pagefault.
     */
    fix_address(mb_info);

    /* allocation memory for paging feature */

    /*
     * Allocation Page_directory_table to avoid wast of memory in this.
     * Bacause size is rounded.
     */
    Paging_data pd = {
        .pdt          = smalloc_page_round(ALL_PAGE_STRUCT_SIZE),
        .p_man        = (Page_manager*)smalloc(sizeof(Page_manager)),
        .p_list_nodes = (List_node*)smalloc(sizeof(List_node) * PAGE_INFO_NODE_NUM),
        .p_info       = (Page_info*)smalloc(sizeof(Page_info) * PAGE_INFO_NODE_NUM),
        .used_p_info  = (bool*)smalloc(sizeof(bool) * PAGE_INFO_NODE_NUM),
    };

    /* initialize dynamic allocated variables. */
    mem_man = smalloc(sizeof(Memory_manager));
    list_init(&mem_man->list, sizeof(Memory_info), NULL);

    mem_info = smalloc(sizeof(Memory_info) * MAX_MEM_NODE_NUM);

    used_list_node = smalloc(sizeof(bool) * MAX_MEM_NODE_NUM);

    /* set memory_info area into list node. */
    mem_list_nodes = smalloc(sizeof(List_node) * MAX_MEM_NODE_NUM);
    for (size_t i = 0; i < MAX_MEM_NODE_NUM; ++i) {
        mem_list_nodes[i].data = &mem_info[i];
    }

    /* FIXME: This is dirty things for vmalloc(). */
    kernel_end_addr = round_page_size(kernel_end_addr);

    /*
     * Set physical 0x000000 to physical kernel end address is allocated.
     * But, there are continuous allocated areas in this area.
     * So, We alloc it to one node.
     * NOTE from http://wiki.osdev.org/Detecting_Memory_(x86)#Memory_Detection_in_Emulators
     *      When you tell an emulator how much memory you want emulated,
     *      the concept is a little "fuzzy" because of the emulated missing bits of RAM below 1M.
     *      If you tell an emulator to emulate 32M, does that mean your address space definitely goes from 0 to 32M -1, with missing bits? Not necessarily.
     *      The emulator might assume that you mean 32M of contiguous memory above 1M, so it might end at 33M -1.
     *      Or it might assume that you mean 32M of total usable RAM, going from 0 to 32M + 384K -1.
     *      So don't be surprised if you see a "detected memory size" that does not exactly match your expectations.
     */
    List_node* kernel_n = list_get_new_memory_node();
    Memory_info* kernel_mi = (Memory_info*)kernel_n->data;
    kernel_mi->base_addr = get_kernel_phys_start_addr();
    kernel_mi->size = get_kernel_phys_end_addr();
    kernel_mi->state = MEM_INFO_STATE_ALLOC;
    list_insert_node_last(&mem_man->list, kernel_n);
    uintptr_t kernel_node_end_addr = kernel_mi->base_addr + kernel_mi->size;

    /* set memory infomation from Multiboot_memory_map. */
    Multiboot_memory_map const* mmap = (Multiboot_memory_map const*)(uintptr_t)mb_info->mmap_addr;
    size_t const mmap_len = mb_info->mmap_length;
    Multiboot_memory_map const* const limit = (Multiboot_memory_map const* const)((uintptr_t)mmap + mmap_len);
    bool first_flag = true;
    for (Multiboot_memory_map const* i = mmap; i < limit; i = (Multiboot_memory_map*)((uintptr_t)i + sizeof(i->size) + i->size)) {
        if ((i->addr + i->len) < kernel_node_end_addr) {
            continue;
        }
        List_node* n = list_get_new_memory_node();

        /* set memory_info */
        Memory_info* mi = (Memory_info*)n->data;
        if (first_flag == true && i->addr <= kernel_node_end_addr) {
            mi->base_addr = kernel_node_end_addr;
            mi->size = (size_t)((uintptr_t)i->len - (kernel_node_end_addr - (uintptr_t)i->addr));
            first_flag = false;
        } else {
            mi->base_addr = (uintptr_t)i->addr;
            mi->size = (size_t)i->len;
        }
        mi->state = (i->type == MULTIBOOT_MEMORY_AVAILABLE) ? MEM_INFO_STATE_FREE : MEM_INFO_STATE_ALLOC;

        /* append to list. */
        list_insert_node_last(&mem_man->list, n);
    }

    /*
     * Physical memory managing is just finished.
     * Next, let's set paging.
     */
    init_paging(&pd);
}


static inline void* smalloc_generic(size_t size, bool is_round_page_size) {
    uintptr_t addr = get_kernel_vir_end_addr();

    if (is_round_page_size == true) {
        uintptr_t raddr = round_page_size(addr);
        size += raddr - addr;
        addr = raddr;
    }

    kernel_end_addr += size; /* size add to kernel_end_addr to allocate memory. */

    memset((void*)addr, 0, size);

    return (void*)addr;
}


/**
 * @brief Strangely memory allocation.
 *        This function allocate memory at kernel tail.
 * @param size allocated memory size.
 * @return pointer to allocated memory.
 */
static inline void* smalloc(size_t size) {
    return smalloc_generic(size, false);
}


static inline void* smalloc_page_round(size_t size) {
    return smalloc_generic(size, true);
}


static size_t for_each_in_malloc_size;
static bool for_each_in_malloc(void* d) {
    Memory_info const* const m = (Memory_info*)d;
    return (m->state == MEM_INFO_STATE_FREE && for_each_in_malloc_size <= m->size) ? true : false;
}


/**
 * @brief Physical memory allocation.
 * @param size allocated memory size.
 * @return pointer to allocated memory.
 */
void* pmalloc(size_t size) {
    /* size_t size = size_byte * 8; */

    /* search enough size node */
    for_each_in_malloc_size = size;
    List_node* n = list_for_each(&mem_man->list, for_each_in_malloc, false);
    if (n == NULL) {
        return NULL;
    }

    /* get new node */
    List_node* new = list_get_new_memory_node();
    if (new == NULL) {
        return NULL;
    }

    /* allocate new area */
    Memory_info* mi = (Memory_info*)n->data;
    Memory_info* new_mi = (Memory_info*)new->data;
    new_mi->base_addr = mi->base_addr + size;
    new_mi->size = mi->size - size;
    new_mi->state = MEM_INFO_STATE_FREE;

    mi->size = size;
    mi->state = MEM_INFO_STATE_ALLOC;

    list_insert_node_next(&mem_man->list, n, new);

    return (void*)mi->base_addr;
}


void* pmalloc_page_round(size_t size) {
    return pmalloc(round_page_size(size));
}


static size_t for_each_in_free_base_addr;
static bool for_each_in_free(void* d) {
    Memory_info const* const m = (Memory_info*)d;
    return (m->state == MEM_INFO_STATE_ALLOC && m->base_addr == for_each_in_free_base_addr) ? true : false;
}


void pfree(void* object) {
    if (object == NULL) {
        return;
    }

    for_each_in_free_base_addr = (uintptr_t)object;
    List_node* n = list_for_each(&mem_man->list, for_each_in_free, false);
    if (n == NULL) {
        return;
    }
    Memory_info* n_mi = (Memory_info*)n->data;

    List_node* const prev_node = n->prev;
    List_node* const next_node = n->next;
    Memory_info* const prev_mi = (Memory_info*)prev_node->data;
    Memory_info* const next_mi = (Memory_info*)next_node->data;

    if (prev_mi->state == MEM_INFO_STATE_FREE && prev_mi->state == MEM_INFO_STATE_FREE) {
        /* merge next and previous. */
        prev_mi->size += (n_mi->size + next_mi->size);
        remove_memory_list_node(n);
        remove_memory_list_node(next_node);
    } else if (prev_mi->state == MEM_INFO_STATE_FREE) {
        /* merge previous. */
        prev_mi->size += n_mi->size;
        remove_memory_list_node(n);
    } else if (next_mi->state == MEM_INFO_STATE_FREE) {
        /* merge next. */
        n_mi->size += next_mi->size;
        remove_memory_list_node(next_node);
    } else {
        n_mi->state = MEM_INFO_STATE_FREE;
    }
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


/**
 * @brief get new list node.
 *      You MUST use this function while managing memory.
 *      Because default get_new_list_node() uses malloc().
 *      And the number of node cannot exceed MAX_MEM_NODE_NUM.
 * @param pointer to List_node.
 * @return new pointer to List_node.
 */
static List_node* list_get_new_memory_node(void) {
    static size_t cnt = 0;         /* counter for nextfix */
    size_t const stored_cnt = cnt; /* It is used for detecting already checking all node. */

    do {
        cnt = MOD_MAX_MEM_NODE_NUM(cnt + 1);
        if (stored_cnt == cnt) {
            /* all node is already used :( */
            return NULL;
        }
    } while (used_list_node[cnt] == true);

    used_list_node[cnt] = true;

    return &mem_list_nodes[cnt];
}


/**
 * @brief remove list node.
 *      You MUST use this function while managing memory.
 *      because default delete_list_node() uses free().
 * @param  target
 * @param  m
 * @return
 */
static void remove_memory_list_node(List_node* target) {
    list_remove_node(&mem_man->list, target);

    /* instead of free(). */
    used_list_node[((uintptr_t)target - (uintptr_t)mem_list_nodes) / sizeof(List_node)] = false;
}
