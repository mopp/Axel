/**
 * @file tlsf.c
 * @brief Two level Segregated Fit allocater implementation.
 * @author mopp
 * @version 0.1
 * @date 2014-09-29
 *
 * NOTE:
 *  First level
 *      2^n < size ≤ 2^(n+1) の n
 *  Second level
 *      L2  2^4 = 16 subdivide.
 *  0 -  1 =    0 -    2
 *  1 -  2 =    2 -    4
 *  2 -  3 =    4 -    8
 *  3 -  4 =    8 -   16
 *  4 -  5 =   16 -   32 (  1 byte * 16)
 *  5 -  6 =   32 -   64 (  2 byte * 16)
 *  6 -  7 =   64 -  128 (  4 byte * 16)
 *  7 -  8 =  128 -  256 (  8 byte * 16)
 *  8 -  9 =  512 - 1024 ( 32 byte * 16)
 *
 *  Under 1024 byte is summarised into one fl list.
 *   0 -  9 =    0 - 1024 ( 64 byte * 16)
 *   9 - 10 = 1024 - 2048 ( 64 byte * 16)
 *  11 - 12 = 2048 - 4096 (128 byte * 16)
 *  12 - 13 = 4096 - 8192 (256 byte * 16)
 */


#include <assert.h>
#include <kernel.h>
#include <paging.h>
#include <stdbool.h>
#include <tlsf.h>
#include <utils.h>


struct block {
    struct block* prev_block; /* Liner previous block */
    Elist list;               /* Logical previous and next block. */
    union {
        struct {
            uint8_t is_free : 1;        /* If current block is free, this is set. */
            uint8_t is_free_prev : 1;   /* If previous block is free.this is set. */
            size_t dummy : (sizeof(size_t) * 8 - 2);
        };
        size_t size;
    };
};
typedef struct block Block;


enum {
    ALIGNMENT_LOG2           = 2,
    ALIGNMENT_SIZE           = PO2(ALIGNMENT_LOG2),
    ALIGNMENT_MASK           = ALIGNMENT_SIZE - 1,

    SL_INDEX_MASK            = PO2(SL_MAX_INDEX_LOG2) - 1,

    FL_BLOCK_MIN_SIZE        = PO2(FL_BASE_INDEX + 1),
    SL_BLOCK_MIN_SIZE_LOG2   = (FL_BASE_INDEX + 1 - SL_MAX_INDEX_LOG2),
    SL_BLOCK_MIN_SIZE        = PO2(SL_BLOCK_MIN_SIZE_LOG2),

    BLOCK_OFFSET             = sizeof(Block),
    BLOCK_FLAG_BIT_FREE      = 0x01,
    BLOCK_FLAG_BIT_PREV_FREE = 0x02,
    BLOCK_FLAG_MASK          = 0x03,

    MAX_ALLOC_ALIGN          = PO2(12),
    MAX_ALLOCATION_SIZE      = FRAME_SIZE * 10,
    WATERMARK_BLOCK_SIZE     = MAX_ALLOC_ALIGN + MAX_ALLOCATION_SIZE,
    WATERMARK_BLOCK_NR_ALLOC = 1,
    WATERMARK_BLOCK_NR_FREE  = 4,

    PAGE_STRUCT_NR           = 50,
};


static Page page_struct_pool[PAGE_STRUCT_NR];


#ifdef NO_OPTIMIZE
#define BIT_NR(type) (sizeof(type) * 8u)
static inline size_t find_set_bit_idx_first(size_t n) {
    size_t mask = 1u;
    size_t idx = 0;
    while (((mask & n) == 0) && (mask <<= 1) != ~0u) {
        ++idx;
    }

    return idx;
}


static inline size_t find_set_bit_idx_last(size_t n) {
    size_t mask = ((size_t)1u << (BIT_NR(size_t) - 1u));
    size_t idx = BIT_NR(size_t);
    do {
        --idx;
    } while (((mask & n) == 0) && (mask >>= 1) != 0);

    return idx;
}


#else
__asm__("find_set_bit_idx_last: \n\t"
        "bsrl 4(%esp), %eax     \n\t"
        "ret                    \n\t");
size_t find_set_bit_idx_last(size_t);


__asm__("find_set_bit_idx_first: \n\t"
        "bsfl 4(%esp), %eax      \n\t"
        "ret                     \n\t");
size_t find_set_bit_idx_first(size_t);
#endif


static inline void set_idxs(size_t size, size_t* fl, size_t* sl) {
    if (size < FL_BLOCK_MIN_SIZE) {
        *fl = 0;
        *sl = size >> (SL_BLOCK_MIN_SIZE_LOG2);
    } else {
        /* Calculate First level index. */
        *fl = find_set_bit_idx_last(size);

        /* Calculate Second level index. */
        *sl = (size >> (*fl - SL_MAX_INDEX_LOG2)) & (SL_INDEX_MASK);

        /* Shift index. */
        *fl -= FL_BASE_INDEX;
    }
}


static inline size_t align_up(size_t x, size_t a) {
    return (x + (a - 1u)) & ~(a - 1u);
}


static inline size_t align_down(size_t x, size_t a) {
    return x & ~(a - 1u);
}


static inline size_t block_align_up(size_t x) {
    return align_up(x, ALIGNMENT_SIZE);
}


static inline size_t adjust_size(size_t size) {
    return block_align_up(size);
}


static inline size_t get_size(Block const* b) {
    return b->size & (~(size_t)BLOCK_FLAG_MASK);
}


static inline void set_size(Block* b, size_t s) {
    b->size = ((b->size & BLOCK_FLAG_MASK) | s);
}


static inline bool is_sentinel(Block const* const b) {
    return (get_size(b) == 0) ? (true) : false;
}


static inline Block* get_phys_next_block(Block const* const b) {
    return (Block*)((uintptr_t)b + (uintptr_t)BLOCK_OFFSET + (uintptr_t)get_size(b));
}


static inline void set_prev_free(Block* b) {
    b->size |= BLOCK_FLAG_BIT_PREV_FREE;
}


static inline void clear_prev_free(Block* b) {
    b->size &= ~(size_t)BLOCK_FLAG_BIT_PREV_FREE;
}


static inline void set_free(Block* b) {
    b->size |= BLOCK_FLAG_BIT_FREE;
    set_prev_free(get_phys_next_block(b));
}


static inline void claer_free(Block* b) {
    b->size &= ~(size_t)BLOCK_FLAG_BIT_FREE;
    clear_prev_free(get_phys_next_block(b));
}


static inline Block* generate_block(void* mem, size_t size) {
    assert((size & ALIGNMENT_MASK) == 0);

    Block* b = mem;
    b->size = size - BLOCK_OFFSET;
    b->prev_block = NULL;
    elist_init(&b->list);

    assert(ALIGNMENT_SIZE <= b->size);

    return b;
}


static inline void* convert_mem_ptr(Block const* b) {
    assert(b != NULL);
    return (void*)((uintptr_t)b + (uintptr_t)BLOCK_OFFSET);
}


static inline Block* convert_block(void const* p) {
    assert(p != NULL);
    return (Block*)((uintptr_t)p - (uintptr_t)BLOCK_OFFSET);
}


static inline Elist* get_block_list_head(Tlsf_manager* const tman, size_t fl, size_t sl) {
    return &tman->blocks[fl * sizeof(Elist) + sl];
}


static inline void insert_block(Tlsf_manager* const tman, Block* b) {
    assert(b != NULL);
    assert(is_sentinel(b) == false);

    size_t fl, sl, s = get_size(b);
    set_idxs(s, &fl, &sl);

    assert(ALIGNMENT_SIZE <= s);

    tman->fl_bitmap      |= PO2(fl);
    tman->sl_bitmaps[fl] |= PO2(sl);

    elist_insert_next(get_block_list_head(tman, fl, sl), &b->list);
}


static inline void sync_bitmap(Tlsf_manager* tman, size_t fl, size_t sl) {
    if (elist_is_empty(get_block_list_head(tman, fl, sl)) == true) {
        uint16_t* sb = &tman->sl_bitmaps[fl];
        *sb &= ~PO2(sl);
        if (*sb == 0) {
            tman->fl_bitmap &= ~PO2(fl);
        }
    }
}


static inline Block* remove_block(Tlsf_manager* tman, Block* b) {
    if (b->is_free == 0) {
        return b;
    }

    size_t fl, sl;
    set_idxs(get_size(b), &fl, &sl);

    elist_remove(&b->list);

    sync_bitmap(tman, fl, sl);

    return b;
}


static inline Block* take_any_block(Tlsf_manager* tman, size_t fl, size_t sl) {
    Elist* head = get_block_list_head(tman, fl, sl);
    assert(elist_is_empty(head) == false);

    Block* b = elist_derive(Block, list, elist_remove(head->next));
    sync_bitmap(tman, fl, sl);

    return b;
}


static inline size_t round_up_block(size_t s) {
    return s + ((FL_BLOCK_MIN_SIZE <= s) ? (PO2(find_set_bit_idx_last(s) - SL_MAX_INDEX_LOG2) - 1u) : (SL_BLOCK_MIN_SIZE));
}


static inline Block* remove_good_block(Tlsf_manager* tman, size_t size) {
    size += BLOCK_OFFSET;

    size = round_up_block(size);

    size_t fl, sl;
    set_idxs(size, &fl, &sl);

    /* Get sl flags equal or greater than current index. */
    size_t sl_map = tman->sl_bitmaps[fl] & (~0u << sl);
    if (sl_map == 0) {
        /*
         * There are not adaptive memory at current fl .
         * Try to get flags of next index.
         */
        size_t fl_map = tman->fl_bitmap & (~0u << (fl + 1u));
        if (fl_map == 0) {
            return NULL;
        }

        /* find big free area. */
        fl = find_set_bit_idx_first(fl_map);

        sl_map = tman->sl_bitmaps[fl];
    }
    /* get available memory in sl. */
    sl = find_set_bit_idx_first(sl_map);

    return take_any_block(tman, fl, sl);
}


/*
 * 引数で与えられたブロックの持つメモリからsize分の新しいブロックを取り出して返す.
 * d
 */
static inline Block* divide_block(Block* b, size_t size, size_t align) {
    assert(b != NULL);
    assert(is_sentinel(b) == false);
    assert(size != 0);

    size_t nblock_all_size = size + BLOCK_OFFSET;
    if (get_size(b) <= nblock_all_size) {
        return NULL;
    }

    Block* old_next = get_phys_next_block(b);

    set_size(b, get_size(b) - nblock_all_size);
    Block* new_next = get_phys_next_block(b);

    if (align != 0) {
        /* mallocの戻り値はオフセット分加算されるのでその分を引いておく. */
        uintptr_t t = (uintptr_t)align_down((size_t)new_next, align) - BLOCK_OFFSET;
        uintptr_t diff = (uintptr_t)new_next - t;

        assert(get_size(b) >= diff);

        size += diff;
        set_size(b, get_size(b) - (size_t)diff);

        new_next = (Block*)t;
    }

    old_next->prev_block = new_next;
    new_next->prev_block = b;

    elist_init(&new_next->list);
    set_size(new_next, size);
    set_free(new_next);
    set_prev_free(new_next);

    return new_next;
}


/*
 * b1とb2を統合する.
 * b2がb1に吸収される形.
 * 物理メモリ上ではb1のアドレスのほうがb2より低い.
 *  -> b1 < b2
 */
static inline void merge_phys_block(Tlsf_manager* tman, Block* b1, Block* b2) {
    assert(b1 < b2);
    assert(((uintptr_t)b1 + BLOCK_OFFSET + (uintptr_t)get_size(b1)) == (uintptr_t)b2);

    remove_block(tman, b1);
    remove_block(tman, b2);

    Block* old_next = get_phys_next_block(b1);
    old_next->prev_block = b1;

    set_size(b1, get_size(b1) + BLOCK_OFFSET + get_size(b2));
    set_prev_free(get_phys_next_block(b1));

    insert_block(tman, b1);
}


static inline Block* merge_phys_next_block(Tlsf_manager* tman, Block* b) {
    Block* next = get_phys_next_block(b);
    if ((is_sentinel(next) == true) || (next->is_free == 0)) {
        return b;
    }

    merge_phys_block(tman, b, next);

    return b;
}


static inline Block* merge_phys_prev_block(Tlsf_manager* tman, Block* b) {
    Block* prev = b->prev_block;
    if (prev == NULL || prev->is_free == 0) {
        return b;
    }

    merge_phys_block(tman, prev, b);

    return prev;
}


static inline Block* merge_phys_neighbor_blocks(Tlsf_manager* tman, Block* b) {
    return merge_phys_prev_block(tman, merge_phys_next_block(tman, b));
}


static inline Page* get_unused_page(void) {
    static size_t page_pool_idx = 0;
    Page* p = &page_struct_pool[page_pool_idx++];

    if (p->addr == 0) {
        return p;
    }

    size_t store = page_pool_idx;
    while (store != ++page_pool_idx) {
        if (PAGE_STRUCT_NR <= page_pool_idx) {
            page_pool_idx = 0;
        }

        p = &page_struct_pool[page_pool_idx];
        if (p->addr == 0) {
            return p;
        }
    }

    return NULL;
}


static inline Tlsf_manager* supply_memory(Tlsf_manager* tman, size_t size) {
    assert((2 * BLOCK_OFFSET) <= size);
    if (size < (2 * BLOCK_OFFSET)) {
        return NULL;
    }

    Page* p = get_unused_page();
    if (p == NULL) {
        return NULL;
    }

    /* Allocate virtual memory. */
    vmalloc(p, size_to_frame_nr(size));
    if (p->frame_nr == 0) {
        return NULL;
    }

    /* Insert to manager and make new block. */
    elist_insert_next(&tman->pages, &p->list);

    size_t ns = (get_page_size(p) - BLOCK_OFFSET);
    Block* new_block = generate_block((void*)p->addr, ns);
    set_free(new_block);

    Block* sentinel = (Block*)(p->addr + (uintptr_t)ns);
    sentinel->prev_block = new_block;
    sentinel->size = 0;

    assert(get_phys_next_block(new_block) == sentinel);

    /*
     * New block is only inserted.
     * Sentinel is NOT logicaly object on physical memory.
     */
    insert_block(tman, new_block);

    ns = get_size(new_block);
    tman->free_memory_size += ns;
    tman->total_memory_size += ns;

    return tman;
}


static inline void check_alloc_watermark(Tlsf_manager* tman, size_t s) {
    size_t const w = block_align_up(WATERMARK_BLOCK_SIZE);
    size_t fl, sl;
    set_idxs(w, &fl, &sl);

    if (tman->free_memory_size < s) {
        size_t t = s + BLOCK_OFFSET * 3;
        void* m = supply_memory(tman, t);
        if (m == NULL) {
            /* printf("alloc failed\n"); */
        }
    } else {
        size_t fl_map = tman->fl_bitmap & (~0u << fl);
        if (fl_map == 0) {
            /* WATERMARK_BLOCK_SIZE以上のブロックが無いので確保. */
            void* m = supply_memory(tman, w + BLOCK_OFFSET * 3);
            if (m == NULL) {
                /* printf("alloc failed\n"); */
            }
        }
    }
}


static inline void check_free_watermark(Tlsf_manager* tman, Block* b) {
    if (b->prev_block != NULL || is_sentinel(get_phys_next_block(b)) == false) {
        return;
    }

    /* Count block that have memory under WATERMARK_BLOCK_SIZE. */
    size_t const w = block_align_up(WATERMARK_BLOCK_SIZE);
    size_t fl, sl, cnt = 1;
    set_idxs(w, &fl, &sl);
    elist_foreach(i, get_block_list_head(tman, fl, sl), Block, list) {
        if (WATERMARK_BLOCK_NR_FREE < cnt++) {
            break;
        }
    }

    if (cnt <= WATERMARK_BLOCK_NR_FREE) {
        return;
    }

    remove_block(tman, b);
    tman->free_memory_size -= get_size(b);
    tman->total_memory_size -= get_size(b);

    Page* p = NULL;
    elist_foreach(i, &tman->pages, Page, list) {
        if (i->addr == (uintptr_t)b) {
            p = i;
        }
    }
    assert(p != NULL);

    elist_remove(&p->list);
    vfree(p);

    memset(p, 0, sizeof(Page));
}


Tlsf_manager* tlsf_init(Tlsf_manager* tman) {
    memset(tman, 0, sizeof(Tlsf_manager));
    elist_init(&tman->pages);
    for (size_t i = 0; i < (FL_MAX_INDEX * SL_MAX_INDEX); i++) {
        elist_init(tman->blocks + i);
    }

    supply_memory(tman, FRAME_SIZE * 6);

    return tman;
}


void tlsf_destruct(Tlsf_manager* tman) {
    if (elist_is_empty(&tman->pages) == true) {
        return;
    }

    elist_foreach(i, &tman->pages, Page, list) {
        Page* p = elist_derive(Page, list, i->list.prev);
        elist_remove(&i->list);
        vfree(p);
        i = p;
    }

    memset(tman, 0, sizeof(Tlsf_manager));
}


inline void* tlsf_malloc_align(Tlsf_manager* tman, size_t size, size_t align) {
    assert((align == 0) || ((align - 1u) & align) == 0);
    assert(align <= MAX_ALLOC_ALIGN);

    if (size == 0 || tman == NULL) {
        return NULL;
    }

    size_t a_size = adjust_size(size + align + BLOCK_OFFSET);

    check_alloc_watermark(tman, a_size);

    Block* gb = remove_good_block(tman, a_size);
    if (gb == NULL) {
        return NULL;
    }

    assert(a_size < get_size(gb));

    Block* sb, * ab = divide_block(gb, adjust_size(size), align);
    if (ab == NULL) {
        /* Directly used because block cannot be divided. */
        sb = gb;
    } else {
        /*
         * Block is divided.
         * Return unused pair block.
         */
        sb = ab;
        insert_block(tman, gb);
        tman->free_memory_size -= BLOCK_OFFSET;
    }

    tman->free_memory_size -= get_size(sb);

    claer_free(sb);
    return convert_mem_ptr(sb);
}


void* tlsf_malloc(Tlsf_manager* tman, size_t size) {
    return tlsf_malloc_align(tman, size, 0);
}


void tlsf_free(Tlsf_manager* tman, void* p) {
    if (tman == NULL || p == NULL) {
        return;
    }

    Block* b = convert_block(p);
    assert(b->is_free == 0);

    set_free(b);

    tman->free_memory_size += (get_size(b) + BLOCK_OFFSET);

    b = merge_phys_neighbor_blocks(tman, b);

    check_free_watermark(tman, b);
}
