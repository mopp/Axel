/*
 * @file include/buddy.h
 * @brief Buddy System header.
 * @author mopp
 * @version 0.2
 * @date 2014-09-23
 */

#ifndef _BUDDY_H_
#define _BUDDY_H_



#include <stdint.h>
#include <stddef.h>
#include <elist.h>

/* Order in buddy system: 0 1 2 3  4  5  6   7   8   9   10 */
/* The number of frame  : 1 2 4 8 16 32 64 128 256 512 1024 */
enum  {
    BUDDY_SYSTEM_MAX_ORDER = (10 + 1),
};


struct frame {
    Elist list;
    uintptr_t mapped_vaddr;
    uint8_t status;
    uint8_t order;
};
typedef struct frame Frame;


enum frame_constants {
    FRAME_STATE_FREE = 0,
    FRAME_STATE_ALLOC,
};


/* Buddy system manager. */
struct buddy_manager {
    Frame* frame_pool;                            /* 管理用の全フレーム */
    uintptr_t base_addr;
    size_t total_frame_nr;                        /* マネージャの持つ全フレーム数 */
    size_t free_frame_nr[BUDDY_SYSTEM_MAX_ORDER]; /* 各オーダーの空きフレーム数 */
    Elist frames[BUDDY_SYSTEM_MAX_ORDER];         /* 各オーダーのリスト先頭要素(ダミー), 実際のデータはこのリストのnext要素から始まる. */
};
typedef struct buddy_manager Buddy_manager;


extern Buddy_manager* buddy_init(Buddy_manager* const, uintptr_t, Frame*, size_t);
extern void buddy_destruct(Buddy_manager* const);
extern Frame* buddy_alloc_frames(Buddy_manager* const, uint8_t);
extern void buddy_free_frames(Buddy_manager* const, Frame*);
extern size_t buddy_get_free_memory_size(Buddy_manager const* const);
extern size_t buddy_get_alloc_memory_size(Buddy_manager const* const);
extern uintptr_t get_frame_addr(Buddy_manager const* const, Frame const* const);
extern Frame* get_frame_by_addr(Buddy_manager const * const, uintptr_t);
extern size_t buddy_get_total_memory_size(Buddy_manager const* const);
extern uint8_t size_to_order(size_t);
extern size_t order_to_size(uint8_t);



#endif
