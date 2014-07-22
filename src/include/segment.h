/**
 * @file include/segment.h
 * @brief segment descriptor header.
 * @author mopp
 * @version 0.1
 * @date 2014-07-22
 */

#ifndef _SEGMENT_H_
#define _SEGMENT_H_



#define KERNEL_CODE_SEGMENT_INDEX 1
#define KERNEL_DATA_SEGMENT_INDEX 2
#define USER_CODE_SEGMENT_INDEX   3
#define USER_DATA_SEGMENT_INDEX   4

#define KERNEL_CODE_SEGMENT_SELECTOR (8 * KERNEL_CODE_SEGMENT_INDEX)
#define KERNEL_DATA_SEGMENT_SELECTOR (8 * KERNEL_DATA_SEGMENT_INDEX)
#define USER_CODE_SEGMENT_SELECTOR   (8 * USER_CODE_SEGMENT_INDEX)
#define USER_DATA_SEGMENT_SELECTOR   (8 * USER_DATA_SEGMENT_INDEX)



#ifndef _ASSEMBLY_H_




struct segment_selector {
    uint16_t request_privilege_level : 2;
    uint16_t table_indicator : 1;
    uint16_t index : 13;
};
typedef struct segment_selector Segment_selector;
_Static_assert(sizeof(Segment_selector) == 2, "Segment_selector size is NOT 16bit");



#endif  /* _ASSEMBLY_H_ */



#endif
