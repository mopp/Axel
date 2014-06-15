/************************************************************
 * File: include/point.h
 * Description: this provides point structure.
 ************************************************************/

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


static inline Point2d* clear_point2d(Point2d* const p) {
    p->x = 0;
    p->y = 0;

    return p;
}


static inline Point2d* set_point2d(Point2d* const p, int32_t x, int32_t y) {
    p->x = x;
    p->y = y;

    return p;
}


static inline Point2d* add_point2d(Point2d* const p, int32_t x, int32_t y) {
    p->x += x;
    p->y += y;

    return p;
}


static inline Point2d* subs_point2d(Point2d* const p, int32_t x, int32_t y) {
    p->x -= x;
    p->y -= y;

    return p;
}


#endif
