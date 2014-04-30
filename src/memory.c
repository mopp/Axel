/************************************************************
 * File: graphic_txt.c
 * Description: some memory functions.
 *      You MUST NOT use get_new_dlinked_list_node().
 *      bacause get_new_dlinked_list_node() include malloc.
 *      So, You MUST use list_get_new_memory_node().
 ************************************************************/

#include <memory.h>
#include <string.h>

/*
 * It is wapped by function. bacause I foget '&'
 * And It is set by linker. see kernel.ld
 */
extern uintptr_t const LD_KERNEL_START;
extern uintptr_t const LD_KERNEL_END;
extern uintptr_t const LD_KERNEL_SIZE;

static uintptr_t kernel_start_addr = (uintptr_t) & LD_KERNEL_START;
static uintptr_t kernel_end_addr = (uintptr_t) & LD_KERNEL_END;
static uintptr_t kernel_size;

/* This variables is dynamicaly allocated in kernel tail. */
static Memory_manager* mem_manager;
/*
 * This pointer to List_node points allocated nodes area in kernel tail.
 * And the number of node is MAX_MEM_NODE_NUM.
 */
static List_node* mem_list_nodes;
/*
 * This pointer to Memory_info points allocated Memory_info area in kernel tail.
 * And the number of node is MAX_MEM_NODE_NUM.
 */
static Memory_info* mem_info;
/*
 * This pointer to bool points allocated Memory_info area in kernel tail.
 * And the number of node is MAX_MEM_NODE_NUM.
 */
static bool* used_list_node;


/**
 * @brief get new list node.
 *      You MUST use this function while managing memory.
 *      because default get_new_list_node() uses malloc().
 *      And The number of node cannot exceed MAX_MEM_NODE_NUM.
 * @param pointer to List_node.
 * @return new pointer to List_node.
 */
static List_node* list_get_new_memory_node(void) {
    /* counter for nextfix */
    static size_t cnt = 0;
    /* It is used for detecting already checking all node. */
    size_t const stored_cnt = cnt;

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
        You MUST use this function while managing memory.
 *      because default delete_list_node() uses free().
 * @param  target
 * @param  m
 * @return
 */
static void remove_memory_list_node(List_node* target) {
    list_remove_node(&mem_manager->list, target);

    /* instead of free(). */
    used_list_node[((uintptr_t)target - (uintptr_t)mem_list_nodes) / sizeof(List_node)] = false;
}


static void dummy_free(void* d) {
    /* dummy free() function for using list_remove_node() in remove_memory_list_node(); */
}


static bool for_each_kernel_area_node(void* d) {
    Memory_info const* const m = (Memory_info*)d;

    if ((get_kernel_start_addr() <= m->base_addr) && (get_kernel_end_addr() <= (m->base_addr + m->size))) {
        return true;
    }

    return false;
}


static size_t for_each_in_malloc_size;
static bool for_each_in_malloc(void* d) {
    Memory_info const* const m = (Memory_info*)d;
    return (m->state == MEM_INFO_STATE_FREE && for_each_in_malloc_size <= m->size) ? true : false;
}


static size_t for_each_in_free_base_addr;
static bool for_each_in_free(void* d) {
    Memory_info const* const m = (Memory_info*)d;
    return (m->state == MEM_INFO_STATE_ALLOC && m->base_addr == for_each_in_free_base_addr) ? true : false;
}


void init_memory(Multiboot_memory_map const* mmap, size_t mmap_len) {
    /* calculate each variable memory address. */
    uintptr_t mem_manager_addr = get_kernel_end_addr();
    uintptr_t mem_list_nodes_addr = mem_manager_addr + sizeof(Memory_manager);
    uintptr_t mem_info_addr = mem_list_nodes_addr + sizeof(List_node) * MAX_MEM_NODE_NUM;
    uintptr_t used_list_node_addr = mem_info_addr + sizeof(Memory_info) * MAX_MEM_NODE_NUM;
    uintptr_t added_variables_end_addr = used_list_node_addr + sizeof(bool) * MAX_MEM_NODE_NUM;

    kernel_end_addr = added_variables_end_addr;        /* update kernel end address. */
    kernel_size = kernel_end_addr - kernel_start_addr; /* update kernel size. */

    /* initialize dynamic allocated variables. */
    mem_manager = (Memory_manager*)mem_manager_addr;
    list_init(&mem_manager->list, sizeof(Memory_info), dummy_free);

    /* clear all memory_info. */
    mem_info = (Memory_info*)mem_info_addr;
    memset(mem_info, 0, sizeof(Memory_info) * MAX_MEM_NODE_NUM);

    /* clear all used_list_node. */
    used_list_node = (bool*)used_list_node_addr;
    memset(used_list_node, false, sizeof(bool) * MAX_MEM_NODE_NUM);

    /* clear and set memory_info area into list node. */
    mem_list_nodes = (List_node*)mem_list_nodes_addr;
    for (size_t i = 0; i < MAX_MEM_NODE_NUM; ++i) {
        mem_list_nodes[i].next = NULL;
        mem_list_nodes[i].prev = NULL;
        mem_list_nodes[i].data = &mem_info[i];
    }

    /* set memory infomation by Multiboot_memory_map. */
    Multiboot_memory_map const* const limit = (Multiboot_memory_map const* const)((uintptr_t)mmap + mmap_len);
    for (Multiboot_memory_map const* i = mmap; i < limit; i = (Multiboot_memory_map*)((uintptr_t)i + sizeof(i->size) + i->size)) {
        List_node* n = list_get_new_memory_node();

        /* set memory_info */
        Memory_info* mi = (Memory_info*)n->data;
        mi->base_addr = (uintptr_t)i->addr;
        mi->size = (size_t)i->len;
        mi->state = (i->type == MULTIBOOT_MEMORY_AVAILABLE) ? MEM_INFO_STATE_FREE : MEM_INFO_STATE_ALLOC;

        /* append to list. */
        list_insert_node_last(&mem_manager->list, n);
    }

    /* allocate kernel area. */
    uintptr_t const ka_start_addr = get_kernel_start_addr(); /* kernel area start address */
    uintptr_t const ka_end_addr = get_kernel_end_addr();     /* kernel area end address */
    uintptr_t const ka_size = ka_end_addr - ka_start_addr;   /* kernel area size */

    /* search karnel area contain node */
    List_node* searched = list_for_each(&mem_manager->list, for_each_kernel_area_node, false);
    if (searched != NULL) {
        Memory_info* m = (Memory_info*)searched->data;

        if ((m->base_addr == ka_start_addr) && (ka_end_addr == (m->base_addr + m->size))) {
            /* just fit ! */
            m->state = MEM_INFO_STATE_ALLOC;
        } else if (m->base_addr == ka_start_addr) {
            /* start is same address, but end in node is shorter than ka_end_addr.  */
            size_t free_area_size = m->size - ka_size;

            m->size = ka_size;
            m->state = MEM_INFO_STATE_ALLOC;

            /* add next free area after kernel area. */
            List_node* n = list_get_new_memory_node();
            Memory_info* free_info = (Memory_info*)n->data;

            free_info->base_addr = ka_start_addr + ka_size;
            free_info->size = free_area_size;
            free_info->state = MEM_INFO_STATE_FREE;

            list_insert_node_next(&mem_manager->list, searched, n);
        } else {
            // TODO: add node include kernel case and node end_addr equals ka_end_addr case.
        }
    }
}


void* malloc(size_t size_byte) {
    size_t size = size_byte * 8;

    /* search enough size node */
    for_each_in_malloc_size = size;
    List_node* n = list_for_each(&mem_manager->list, for_each_in_malloc, false);
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

    list_insert_node_next(&mem_manager->list, n, new);

    return (void*)mi->base_addr;
}


void free(void* object) {
    for_each_in_free_base_addr = (uintptr_t)object;
    List_node* n = list_for_each(&mem_manager->list, for_each_in_free, false);
    if (n == NULL) {
        return;
    }
    Memory_info* n_mi = (Memory_info*)n->data;

    List_node* prev_node = n->prev;
    List_node* next_node = n->next;
    Memory_info* prev_mi = (Memory_info*)prev_node->data;
    Memory_info* next_mi = (Memory_info*)next_node->data;

    if (prev_mi->state == MEM_INFO_STATE_FREE) {
        /* merge previous. */
        prev_mi->size += n_mi->size;
        remove_memory_list_node(n);
    } else if (next_mi->state == MEM_INFO_STATE_FREE) {
        /* merge next. */
        next_mi->base_addr -= n_mi->size;
        next_mi->size += n_mi->size;
        remove_memory_list_node(n);
    } else {
        n_mi->state = MEM_INFO_STATE_FREE;
    }
}


uintptr_t get_kernel_start_addr(void) {
    return kernel_start_addr;
}


uintptr_t get_kernel_end_addr(void) {
    return kernel_end_addr;
}


uintptr_t get_kernel_size(void) {
    return kernel_size;
}
