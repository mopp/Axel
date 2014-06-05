/**
 * @file atype.h
 * @brief type definition header.
 * @author mopp
 * @version 0.1
 * @date 2014-05-01
 */

#ifndef _ATYPE_H_
#define _ATYPE_H_


/*
 * free function for list node.
 * It is used in list_destruct().
 */
typedef void (*release_func)(void*);

/*
 * comparison function for list node.
 * It is used in search_list_node().
 */
typedef bool (*comp_func)(void*, void*);

/*
 * for each function for list node.
 * if return value is true, loop is abort
 */
typedef bool (*for_each_func)(void*);



#endif
