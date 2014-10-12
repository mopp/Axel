/**
 * @file include/point.h
 * @brief this provides point structure.
 * @author mopp
 * @version 0.1
 * @date 2014-10-13
 */


#ifndef _POINT_H_
#define _POINT_H_



#include <stdint.h>


struct point2d {
    int32_t x, y;
};
typedef struct point2d Point2d;


#define make_point2d(ix, iy) \
    (Point2d) {              \
        .x = ix, .y = iy     \
    }


Point2d* clear_point2d(Point2d* const);
Point2d* set_point2d(Point2d* const, int32_t, int32_t);
Point2d* add_point2d(Point2d* const, int32_t, int32_t);
Point2d* subs_point2d(Point2d* const, int32_t, int32_t);



#endif
