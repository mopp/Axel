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

/*
 * Order in buddy system : 0  1  2  3  4  5  6   7   8   9   10   11   12   13    14
 * The number of frame   : 1  2  4  8 16 32 64 128 256 512 1024 2048 4096 8192 16384
 */
enum  {
    BUDDY_SYSTEM_MAX_ORDER = (14 + 1),
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


struct buddy_manager {
    Frame* frame_pool;                            /* All managed frames */
    uintptr_t base_addr;
    size_t total_frame_nr;                        /* The number of all frame under the manager. */
    size_t free_frame_nr[BUDDY_SYSTEM_MAX_ORDER]; /* The number of free frames each order. */
    Elist frames[BUDDY_SYSTEM_MAX_ORDER];         /* Head of list each order of frame. */
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
