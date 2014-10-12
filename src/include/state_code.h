/**
 * @file include/state_code.h
 * @brief There are Axel StateCode.
 * @author mopp
 * @version 0.1
 * @date 2014-10-13
 */


#ifndef _STATE_CODE_H_
#define _STATE_CODE_H_



enum axel_state_code {
    AXEL_SUCCESS = 0x01,
    AXEL_FAILED,
    AXEL_MEMORY_ALLOC_ERROR,
    AXEL_FRAME_ALLOC_ERROR,
    AXEL_ERROR_INITIALIZE_MEMORY,
    AXEL_PAGE_SYNC_ERROR,
};
typedef enum axel_state_code Axel_state_code;



#endif
