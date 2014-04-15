/************************************************************
 * File: graphic_txt.c
 * Description: some memory functions.
 *      You MUST NOT use get_new_Dlinked_list_node().
 *      bacause get_new_Dlinked_list_node() include malloc.
 ************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <memory.h>
#include <doubly_linked_list.h>
#include <stdbool.h>


/* &をよく忘れるので関数で包む */
extern const uintptr_t LD_KERNEL_SIZE;
extern const uintptr_t LD_KERNEL_START;
extern const uintptr_t LD_KERNEL_END;

/* variable order in memory is mem_manager, dlst_nodes, mem_info. */
static Memory_manager* mem_manager;
static struct dlinked_list_node* dlst_nodes;
static Memory_info* mem_info;
static bool node_used[MAX_MEM_NODE_NUM];    // TODO: use memory allocate


uintptr_t get_kernel_start_addr(void) {
    return (uintptr_t) & LD_KERNEL_START;
}


uintptr_t get_kernel_end_addr(void) {
    return (uintptr_t) & LD_KERNEL_END;
}


uintptr_t get_kernel_size(void) {
    return (uintptr_t) & LD_KERNEL_SIZE;
}


static Dlinked_list_node get_new_memory_list_node(void) {
    /* counter for nextfix */
    static int cnt = -1;
    /* for already checking all node. */
    bool is_one_round = false;

    do {
        ++cnt;
        if (MAX_MEM_NODE_NUM <= cnt) {
            cnt = 0;
            if (is_one_round == true) {
                /* All node is already used. */
                return NULL;
            }
            is_one_round = true;
        }
    } while (node_used[cnt] == true);

    node_used[cnt] = true;

    return &dlst_nodes[cnt];
}


static void delete_memory_list_node(Dlinked_list_node del) {
    node_used[((uintptr_t)del - (uintptr_t)dlst_nodes) / sizeof(struct dlinked_list_node)] = false;
    delete_node(del);
}


static inline void set_meminfo(Multiboot_memory_map* mmap, Memory_info* mi) {
    mi->base_addr = (uintptr_t)mmap->addr;
    mi->size = (uintptr_t)mmap->len;
    mi->state = (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) ? MEM_INFO_STATE_FREE : MEM_INFO_STATE_ALLOC;
}


#ifdef DEBUG

static void print_memory_list(void) {
    Dlinked_list_node t = mem_manager->mem_lst;
    Memory_info* mi = t->data;

    while (t->tail != DUMMY) {
        printf("addr: 0x%x, ", mi->base_addr);
        printf("size: 0x%x, ", mi->size);
        printf("state: 0x%x\n", mi->state);

        t = t->tail;
        mi = t->data;
    }
}

#endif


void init_memory(Multiboot_memory_map* mmap, size_t mmap_len) {
    uintptr_t const t = get_kernel_end_addr();

    /* allocate dlst_nodes after kernel_end + memory_manager addres */
    dlst_nodes = (Dlinked_list_node)(t + sizeof(Memory_manager));
    mem_info = (Memory_info*)(dlst_nodes + (sizeof(struct dlinked_list_node) * MAX_MEM_NODE_NUM));

    for (int i = 0; i < MAX_MEM_NODE_NUM; ++i) {
        /* 各ノードにメモリ情報を付加 */
        /* この時点でノードの先頭と末尾は何も指していない */
        init_list(&dlst_nodes[i], (uintptr_t)(mem_info + i));

        /* meminfo 初期化 */
        memset(mem_info + i, 0, sizeof(Memory_info));
    }

    /* memory_manager is allocated at kernel_end address. */
    mem_manager = (Memory_manager*)t;
    mem_manager->mem_lst = &dlst_nodes[0];
    mem_manager->exist_info_num = 1;
    mem_manager->lost_times = 0;
    mem_manager->lost_size = 0;
    node_used[0] = true;

    set_meminfo(mmap, (Memory_info*)mem_manager->mem_lst->data);

    Dlinked_list_node tdl = mem_manager->mem_lst;

    Multiboot_memory_map const * const limit = (Multiboot_memory_map*)((uintptr_t)mmap + mmap_len);
    while (mmap < limit) {
        /* 次の要素へ */
        mmap = (Multiboot_memory_map*)((uintptr_t)mmap + sizeof(mmap->size) + mmap->size);

        Dlinked_list_node dl = get_new_memory_list_node();
        Memory_info* mi = (Memory_info*)dl->data;

        set_meminfo(mmap, mi);

        tdl = insert_tail(tdl, dl);
    }

    /* allocate kernel area node */
    Dlinked_list_node ka_node = mem_manager->mem_lst;
    Memory_info* ka_mi = (Memory_info*)ka_node->data;

    /* kernel area start address */
    uintptr_t const ka_start_addr = get_kernel_start_addr();
    /* kernel area end address */
    uintptr_t const ka_end_addr = t + (uintptr_t)mem_info + sizeof(Memory_info) * MAX_MEM_NODE_NUM;
    /* kernel area size */
    uintptr_t const ka_size = ka_end_addr - ka_start_addr;

    /* search karnel area contain node */
    while (ka_node->tail != DUMMY) {
        if (ka_mi->base_addr <= ka_start_addr && ka_end_addr <= (ka_mi->base_addr + ka_mi->size)) {
            break;
        }

        ka_node = ka_node->tail;
        ka_mi = (Memory_info*)ka_node->data;
    }

    if (ka_mi->base_addr == ka_start_addr && (ka_mi->base_addr + ka_mi->size) == ka_end_addr) {
        ka_mi->state = MEM_INFO_STATE_ALLOC;
        return;
    }

    if (ka_mi->base_addr == ka_start_addr) {
        ka_mi->size = ka_size;
        ka_mi->state = MEM_INFO_STATE_ALLOC;

        Dlinked_list_node ka_new_node = get_new_memory_list_node();
        Memory_info* ka_new_mi = (Memory_info*)ka_new_node->data;

        ka_new_mi->base_addr = ka_mi->base_addr + ka_size;
        ka_new_mi->size = ka_mi->size - ka_size;
        ka_new_mi->state = MEM_INFO_STATE_FREE;

        insert_tail(ka_node, ka_new_node);
    } else {
        // TODO
    }
}


void* malloc(size_t size_byte) {
    size_t size = size_byte * 8;

    Dlinked_list_node lst = mem_manager->mem_lst;
    Memory_info* mi = (Memory_info*)lst->data;

    /* search enough size node */
    while (lst->tail != DUMMY) {
        if (mi->state == MEM_INFO_STATE_FREE && size < mi->size) {
            break;
        }

        lst = lst->tail;
        mi = (Memory_info*)lst->data;
    }

    /* get new node */
    Dlinked_list_node new = get_new_memory_list_node();
    if (new == NULL) {
        return NULL;
    }

    Memory_info* new_mi = (Memory_info*)new->data;
    new_mi->base_addr = mi->base_addr + size;
    new_mi->size = mi->size - size;
    new_mi->state = MEM_INFO_STATE_FREE;

    mi->size = size;
    mi->state = MEM_INFO_STATE_ALLOC;

    insert_tail(lst, new);

    return (void*)(mi->base_addr);
}


void free(void* object) {
    uintptr_t allocated_addr = (uintptr_t)object;
    Dlinked_list_node lst = mem_manager->mem_lst;
    Memory_info* mi = (Memory_info*)lst->data;

    while (lst->tail != DUMMY) {
        if (mi->base_addr == allocated_addr) {
            break;
        }

        lst = lst->tail;
        mi = (Memory_info*)lst->data;
    }

    Dlinked_list_node head = lst->head, tail = lst->tail;
    Memory_info* head_mi = (Memory_info*)head->data, *tail_mi = (Memory_info*)tail->data;

    if (head_mi->state == MEM_INFO_STATE_FREE) {
        // merge head
        head_mi->size += mi->size;
        delete_memory_list_node(lst);
    } else if (tail_mi->state == MEM_INFO_STATE_FREE) {
        if (lst == mem_manager->mem_lst) {
            mem_manager->mem_lst = lst->tail;
        }

        // merge tail
        tail_mi->base_addr -= mi->size;
        tail_mi->size += mi->size;
        delete_memory_list_node(lst);
    } else {
        mi->state = MEM_INFO_STATE_FREE;
    }
}



int memcmp(const void* buf1, const void* buf2, size_t len) {
    const unsigned char* ucb1, *ucb2, *t;
    int result = 0;
    ucb1 = buf1;
    ucb2 = buf2;
    t = ucb2 + len;

    while (t != buf2 && result == 0) {
        result = *ucb1++ - *ucb2++;
    }

    return result;
}


void* memset(void* buf, const int ch, size_t len) {
    uintptr_t end = (uintptr_t)buf + len;
    uintptr_t t = (uintptr_t)buf;

    while (t < end) {
        *(uint8_t*)t++ = (uint8_t)ch;
    }

    return buf;
}
