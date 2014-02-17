/************************************************************
 * File: graphic_txt.c
 * Description: some memory functions.
 ************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <memory.h>
#include <doubly_linked_list.h>
/* #include <graphic_txt.h> */


/* &をよく忘れるので関数で包む */
extern const uint32_t LD_KERNEL_SIZE;
extern const uint32_t LD_KERNEL_START;
extern const uint32_t LD_KERNEL_END;

static Memory_manager* mem_manager;
static struct dlinked_list_node* dlst_nodes;
static Memory_info* mem_info;


uint32_t get_kernel_start_addr(void) {
    return (uint32_t)&LD_KERNEL_START;
}


uint32_t get_kernel_end_addr(void) {
    return (uint32_t)&LD_KERNEL_END;
}


uint32_t get_kernel_size(void) {
    return (uint32_t)&LD_KERNEL_SIZE;
}


static Dlinked_list_node get_new_memory_list_node(void) {
    static int lst_counter = 0;

    // empty
    if (lst_counter == MAX_MEM_NODE_NUM) {
        return NULL;
    }

    // FIXME
    return &dlst_nodes[++lst_counter];
}


static inline void init_memory_list(void) {
    dlst_nodes = (struct dlinked_list_node*)(get_kernel_end_addr() + sizeof(Memory_manager));
    mem_info = (Memory_info*)(dlst_nodes + sizeof(struct dlinked_list_node) * MAX_MEM_NODE_NUM);

    for (int i = 0; i < MAX_MEM_NODE_NUM; ++i) {
        // 各ノードにメモリ情報を付加
        // この時点でメモリの先頭と末尾は何も指していない
        init(&dlst_nodes[i], mem_info + i);
        memset(mem_info + i, 0, sizeof(Memory_info));
    }
}


static inline void set_meminfo(Multiboot_memory_map* mmap, Memory_info* mi) {
    mi->base_addr = mmap->addr;
    mi->size = mmap->len;
    mi->state = (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) ? MEM_INFO_STATE_FREE : MEM_INFO_STATE_ALLOC;
}

/*
 *
 * static void print_memory_list(void) {
 *     Dlinked_list_node t = mem_manager->mem_lst;
 *     Memory_info* mi = t->data;
 *
 *     while (t->tail != DUMMY) {
 *         printf("addr: 0x%x, ", mi->base_addr);
 *         printf("size: 0x%x, ", mi->size);
 *         printf("state: 0x%x\n", mi->state);
 *
 *         t = t->tail;
 *         mi = t->data;
 *     }
 * }
 */


void init_memory(Multiboot_memory_map* mmap, size_t mmap_len) {
    Multiboot_memory_map* limit = (Multiboot_memory_map*)((uint32_t)mmap + mmap_len);

    init_memory_list();

    // マネージャにカーネル領域末尾を使用
    mem_manager = (Memory_manager*)get_kernel_end_addr();
    mem_manager->mem_lst = &dlst_nodes[0];
    mem_manager->exist_info_num = 1;
    mem_manager->lost_times = 0;
    mem_manager->lost_size = 0;

    set_meminfo(mmap, mem_manager->mem_lst->data);

    Dlinked_list_node tdl = mem_manager->mem_lst;

    while (mmap < limit) {
        // 次の要素へ
        mmap = (Multiboot_memory_map*)((uint32_t)mmap + sizeof(mmap->size) + mmap->size);

        Dlinked_list_node dl = get_new_memory_list_node();
        Memory_info* mi = dl->data;

        set_meminfo(mmap, mi);

        tdl = insert_tail(tdl, dl);
    }

    // allocate kernel area
    Dlinked_list_node ka_node = mem_manager->mem_lst;
    Memory_info* ka_mi = ka_node->data;
    const uint32_t ka_start_addr = get_kernel_start_addr();
    const uint32_t ka_end_addr = get_kernel_end_addr() + (uint32_t)mem_info + sizeof(Memory_info) * MAX_MEM_NODE_NUM;
    const uint32_t ka_size = ka_end_addr - ka_start_addr;

    while (ka_node->tail != DUMMY) {
        if (ka_mi->base_addr <= ka_start_addr && ka_end_addr <= (ka_mi->base_addr + ka_mi->size)) {
            break;
        }

        ka_node = ka_node->tail;
        ka_mi = ka_node->data;
    }

    if (ka_mi->base_addr == ka_start_addr && (ka_mi->base_addr + ka_mi->size) == ka_end_addr) {
        ka_mi->state = MEM_INFO_STATE_ALLOC;
        return;
    }

    if (ka_mi->base_addr == ka_start_addr) {
        ka_mi->size = ka_size;
        ka_mi->state = MEM_INFO_STATE_ALLOC;

        Dlinked_list_node ka_new_node = get_new_memory_list_node();
        Memory_info* ka_new_mi = ka_new_node->data;

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
    Memory_info* mi = lst->data;

    while (lst->tail != DUMMY) {
        if (mi->state == MEM_INFO_STATE_FREE && size < mi->size) {
            break;
        }

        lst = lst->tail;
        mi = lst->data;
    }

    Dlinked_list_node new = get_new_memory_list_node();
    if (new == NULL) {
        return NULL;
    }
    Memory_info* new_mi = new->data;
    new_mi->base_addr = mi->base_addr + size;
    new_mi->size = mi->size - size;
    new_mi->state = MEM_INFO_STATE_FREE;

    mi->size = size;
    mi->state = MEM_INFO_STATE_ALLOC;

    insert_tail(lst, new);

    return (void*)(mi->base_addr);
}


void free(void* object) {
    uint32_t allocated_addr = (uint32_t)object;
    Dlinked_list_node lst = mem_manager->mem_lst;
    Memory_info* mi = lst->data;

    while (lst->tail != DUMMY) {
        if (mi->base_addr == allocated_addr) {
            break;
        }

        lst = lst->tail;
        mi = lst->data;
    }

    Dlinked_list_node head = lst->head, tail = lst->tail;
    Memory_info *head_mi = (Memory_info*)head->data, *tail_mi = (Memory_info*)tail->data;

    if (head_mi->state == MEM_INFO_STATE_FREE) {
        // merge head
        head_mi->size += mi->size;
        delete_node(lst);
    } else if (tail_mi->state == MEM_INFO_STATE_FREE) {
        if (lst == mem_manager->mem_lst) {
           mem_manager->mem_lst = lst->tail;
        }

        // merge tail
        tail_mi->base_addr -= mi->size;
        tail_mi->size += mi->size;
        delete_node(lst);
    } else {
        mi->state = MEM_INFO_STATE_FREE;
    }
}



int memcmp(const void* buf1, const void* buf2, size_t len) {
    const unsigned char *ucb1, *ucb2, *t;
    int result = 0;
    ucb1 = buf1;
    ucb2 = buf2;
    t = buf2 + len;

    while (t != buf2 && result == 0) {
        result = *ucb1++ - *ucb2++;
    }

    return result;
}


void* memset(void* buf, const int ch, size_t len) {
    char* end = buf + len;
    char* t = buf;

    while (t < end) {
        *t++ = ch;
    }

    return buf;
}
