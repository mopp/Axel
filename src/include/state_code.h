/************************************************************
 * File: include/state_code.h
 * Description: There are Axel StateCode.
 ************************************************************/

#ifndef _STATE_CODE_H_
#define _STATE_CODE_H_



enum axel_state_code {
    AXEL_SUCCESS = 0x01,
    AXEL_FAILED,
    AXEL_MEMORY_ALLOC_ERROR,
    AXEL_PAGE_SYNC_ERROR,
};
typedef enum axel_state_code Axel_state_code;



#endif
