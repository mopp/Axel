/**
 * @file point.c
 * @brief point implementation.
 * @author mopp
 * @version 0.1
 * @date 2014-10-13
 */


#include <point.h>


Point2d* clear_point2d(Point2d* const p) {
    p->x = 0;
    p->y = 0;

    return p;
}


Point2d* set_point2d(Point2d* const p, int32_t x, int32_t y) {
    p->x = x;
    p->y = y;

    return p;
}


Point2d* add_point2d(Point2d* const p, int32_t x, int32_t y) {
    p->x += x;
    p->y += y;

    return p;
}


Point2d* subs_point2d(Point2d* const p, int32_t x, int32_t y) {
    p->x -= x;
    p->y -= y;

    return p;
}
