/**
 * @file buddy.c
 * @brief Buddy System implementation.
 * @author mopp
 * @version 0.1
 * @date 2014-09-23
 */

#include <assert.h>
#include <elist.h>
#include <buddy.h>
#include <stdbool.h>
#include <paging.h>
#include <utils.h>
#include <macros.h>


static inline Frame* elist_get_frame(Elist* const l) {
    return (Frame*)l;
}


static inline size_t get_order_frame_size(uint8_t o) {
    return PO2(o) << FRAME_SIZE_LOG2;
}


/**
 * @brief Calculate frame index.
 * @param bman  manager which have frame in argument.
 * @param frame frame for calculating frame.
 * @return index of frame.
 */
static inline size_t get_frame_idx(Buddy_manager const* const bman, Frame const* const frame) {
    assert(bman != NULL);
    assert(frame != NULL);
    assert((uintptr_t)bman->frame_pool <= (uintptr_t)frame);
    return ((uintptr_t)frame - (uintptr_t)bman->frame_pool) / sizeof(Frame);
}


/**
 * @brief バディのアドレスが2の累乗であることとxorを利用して、フレームのバディを求める関数.
 *          xor は そのビットが1であれば0に、0であれば1にする.
 *          ---------------------
 *          | Buddy A | Buddy B |
 *          ---------------------
 *          上図において、引数がBuddy Aなら足して、Buddy Bを求める.
 *          Buddy Bなら引いて、Buddy Aを求めるという処理になる.
 *          また、オーダーが1の時、要素0番のバディは要素2番である.
 *          0 + (1 << 1) = 2
 * @param bman  フレームの属するマネージャ.
 * @param frame バディを求めたいフレーム.
 * @param order バディを求めるオーダー.
 * @return バディが存在しない場合NULLが返る.
 */
static inline Frame* get_buddy_frame(Buddy_manager const* const bman, Frame const* const frame, uint8_t order) {
    Frame* p = bman->frame_pool;
    Frame* f = p + (get_frame_idx(bman, frame) ^ PO2(order));
    return ((p + bman->total_frame_nr) <= f) ? NULL : f;
}


/**
 * @brief バディマネージャを初期化.
 * @param bman target for initiation.
 * @param base base address.
 * @param frames pointer to frame array.
 * @param frame_nr the number of frames.
 * @return If error occur, It returns NULL, otherwise return bman.
 */
Buddy_manager* buddy_init(Buddy_manager* const bman, uintptr_t base, Frame* frames, size_t frame_nr) {
    assert(bman != NULL);
    assert(memory_size != 0);
    assert(frame_nr != 0);

    /* 確保した全フレーム初期化. */
    Frame* p = frames;
    Frame* end = frames + frame_nr;
    do {
        p->status = FRAME_STATE_FREE;
    } while (++p <= end);

    /* マネージャを初期化 */
    bman->frame_pool = frames;
    bman->total_frame_nr = frame_nr;
    bman->base_addr = base;
    for (uint8_t i = 0; i < BUDDY_SYSTEM_MAX_ORDER; ++i) {
        bman->free_frame_nr[i] = 0;
        elist_init(bman->frames + i);
    }

    /* フレームを大きいオーダーからまとめてリストを構築. */
    size_t n = frame_nr;
    uint8_t order = BUDDY_SYSTEM_MAX_ORDER;
    Frame* itr = frames;
    do {
        --order;
        size_t o_nr = PO2(order);
        while (n != 0 && o_nr <= n) {
            /* フレームを現在オーダのリストに追加. */
            itr->order  = order;
            itr->status = FRAME_STATE_FREE;
            elist_insert_next(&bman->frames[order], &itr->list);
            ++(bman->free_frame_nr[order]);

            itr += o_nr; /* 次のフレームへ. */
            n -= o_nr;   /* 取ったフレーム分を引く. */
        }
    } while (0 < order);

    return bman;
}


/**
 * @brief destruct buddy manager.
 * @param bman destruct target manager.
 */
void buddy_destruct(Buddy_manager* const bman) {
    memset(bman, 0, sizeof(Buddy_manager));
}


/**
 * @brief allocate frame by order.
 * @param bman          buddy manager.
 * @param request_order allocate order.
 * @return If this cannot allocate, this will return NULL.
 */
Frame* buddy_alloc_frames(Buddy_manager* const bman, uint8_t request_order) {
    assert(bman != NULL);
    Elist* frames = bman->frames;

    /* O(10 * 10) ? */
    uint8_t order = request_order;
    while (order < BUDDY_SYSTEM_MAX_ORDER) {
        Elist* l = &frames[order];
        if (elist_is_empty(l) == false) {
            --bman->free_frame_nr[order];
            Frame* rm_frame = elist_get_frame(elist_remove(l->next));
            rm_frame->order = request_order;
            rm_frame->status = FRAME_STATE_ALLOC;

            /* 要求オーダーよりも大きいオーダーからフレームを取得した場合、余分なフレームを繋ぎ直す. */
            while (request_order < order--) {
                Frame* bf = get_buddy_frame(bman, rm_frame, order); /* 2分割 */
                bf->order = order;                                  /* 分割したのでバディのオーダーを設定.これをやらないと解放時に困る. */
                elist_insert_next(&frames[order], &bf->list);       /* バディを一つしたのオーダーのリストへ接続 */
                ++bman->free_frame_nr[order];
            }

            return rm_frame;
        }
        /* requested order is NOT found. */

        ++order;
    }

    /* Error */
    return NULL;
}


/**
 * @brief free frames.
 * @param bman destination manager of freed frame.
 * @param ffs  freed frames.
 */
void buddy_free_frames(Buddy_manager* const bman, Frame* ffs) {
    Frame* bf;
    uint8_t order = ffs->order;

    // 開放するフレームのバディが空きであれば、2つを合わせる.
    while ((order < (BUDDY_SYSTEM_MAX_ORDER - 1)) && ((bf = get_buddy_frame(bman, ffs, order)) != NULL) && (order == bf->order) && (bf->status == FRAME_STATE_FREE)) {
        elist_remove(&bf->list);
        --bman->free_frame_nr[bf->order];
        ++order;
    }

    ++bman->free_frame_nr[order];
    ffs->order = order;
    ffs->ref_count = 0;
    ffs->status = FRAME_STATE_FREE;
    ffs->mapped_kvaddr = 0;
    elist_insert_next(&bman->frames[order], &ffs->list);
}


/**
 * @brief Calculate free memory size of BuddySystem.
 * @param bman buddy manager.
 * @return free memory size.
 */
size_t buddy_get_free_memory_size(Buddy_manager const* const bman) {
    size_t free_mem_size = 0;
    for (uint8_t i = 0; i < BUDDY_SYSTEM_MAX_ORDER; i++) {
        free_mem_size += (bman->free_frame_nr[i] * get_order_frame_size(i));
    }

    return free_mem_size;
}


/**
 * @brief Calculate alloc memory size of BuddySystem.
 * @param bman buddy manager
 * @return current memory usage.
 */
size_t buddy_get_alloc_memory_size(Buddy_manager const* const bman) {
    return (bman->total_frame_nr << FRAME_SIZE_LOG2) - buddy_get_free_memory_size(bman);
}


/**
 * @brief Calculate total memory size.
 * @param bman buddy manager.
 * @return total memory size.
 */
size_t buddy_get_total_memory_size(Buddy_manager const* const bman) {
    return bman->total_frame_nr << FRAME_SIZE_LOG2;
}


/**
 * @brief Calculate frame address.
 * @param bman  manager which have frame in argument.
 * @param frame frame for calculating address.
 * @return address of frame.
 */
uintptr_t frame_to_phys_addr(Buddy_manager const* const bman, Frame const* const frame) {
    assert(bman != NULL);
    assert(frame != NULL);
    return bman->base_addr + (get_frame_idx(bman, frame) * FRAME_SIZE);
}


Frame* phys_addr_to_frame(Buddy_manager const * const bman, uintptr_t addr) {
    return &bman->frame_pool[(addr - bman->base_addr) >> FRAME_SIZE_LOG2];
}


static size_t const order_nr[BUDDY_SYSTEM_MAX_ORDER] = {
    FRAME_SIZE * 1,
    FRAME_SIZE * 2,
    FRAME_SIZE * 4,
    FRAME_SIZE * 8,
    FRAME_SIZE * 16,
    FRAME_SIZE * 32,
    FRAME_SIZE * 64,
    FRAME_SIZE * 128,
    FRAME_SIZE * 256,
    FRAME_SIZE * 512,
    FRAME_SIZE * 1024,
    FRAME_SIZE * 2048,
    FRAME_SIZE * 4096,
    FRAME_SIZE * 8192,
    FRAME_SIZE * 16384,
};
uint8_t size_to_order(size_t s) {
    assert(s <= order_nr[BUDDY_SYSTEM_MAX_ORDER - 1]);

    for (uint8_t i = 0; i < BUDDY_SYSTEM_MAX_ORDER; i++) {
        if (s <= order_nr[i]) {
            return i;
        }
    }

    assert(0);
    return 0;
}


size_t order_to_size(uint8_t o) {
    assert(o < BUDDY_SYSTEM_MAX_ORDER);
    return order_nr[o];
}
