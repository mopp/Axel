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
#include <string.h>
#include <stdbool.h>


/* FIXME: */
#define FRAME_SIZE 0x1000U
#define ORDER_NUMBER(order) (1U << (order))
#define ORDER_FRAME_SIZE(order) (ORDER_NUMBER(order) * FRAME_SIZE)


static inline Frame* elist_get_frame(Elist const* const l) {
    return (Frame*)l;
}


/**
 * @brief フレームのindexを求める..
 * @param bman  フレームの属するマネージャ.
 * @param frame indexを求めるフレーム.
 * @return フレームのindex.
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
    Frame* f = p + (get_frame_idx(bman, frame) ^ ORDER_NUMBER(order));
    return ((p + bman->total_frame_nr) <= f) ? NULL : f;
}


/**
 * @brief バディマネージャを初期化.
 * @param bman        初期化対象
 * @param memory_size バディマネージャの管理するメモリーサイズ.
 * @return 初期化出来なかった場合NULL, それ以外は引数のマネージャが返る.
 */
Buddy_manager* buddy_init(Buddy_manager* const bman, Frame* frames, size_t frame_nr) {
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
        size_t o_nr = ORDER_NUMBER(order);
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
 * @brief バディマネージャを破棄.
 * @param bman        破棄対象
 */
void buddy_destruct(Buddy_manager* const bman) {
    memset(bman, 0, sizeof(Buddy_manager));
}


/**
 * @brief 指定オーダーのフレームを確保する.
 * @param bman          確保先のフレームを持つマネージャ.
 * @param request_order 確保するオーダー.
 * @return 確保出来なかった場合NULLが返る.
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
 * @brief フレームを解放する.
 * @param bman フレームの返却先マネージャ.
 * @param ffs  解放するフレーム.
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
    ffs->status = FRAME_STATE_FREE;
    elist_insert_next(&bman->frames[order], &ffs->list);
}


/**
 * @brief マネージャ管理下の空きメモリ容量を求める.
 * @param bman 求める対象のマネージャ.
 * @return 空きメモリ容量.
 */
size_t buddy_get_free_memory_size(Buddy_manager const* const bman) {
    size_t free_mem_size = 0;
    for (uint8_t i = 0; i < BUDDY_SYSTEM_MAX_ORDER; i++) {
        free_mem_size += (bman->free_frame_nr[i] * ORDER_FRAME_SIZE(i));
    }

    return free_mem_size;
}


/**
 * @brief マネージャ管理下の使用メモリ容量を求める.
 * @param bman 求める対象のマネージャ.
 * @return 使用メモリ容量.
 */
size_t buddy_get_alloc_memory_size(Buddy_manager const* const bman) {
    return (bman->total_frame_nr * FRAME_SIZE) - buddy_get_free_memory_size(bman);
}


/**
 * @brief フレームのアドレスを求める..
 * @param bman  フレームの属するマネージャ.
 * @param frame アドレスを求めるフレーム.
 * @return フレームのアドレス.
 */
uintptr_t get_frame_addr(Buddy_manager const* const bman, Frame const* const frame) {
    assert(bman != NULL);
    assert(frame != NULL);
    return get_frame_idx(bman, frame) * FRAME_SIZE;
}
