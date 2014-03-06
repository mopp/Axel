/************************************************************
 * File: point.c
 * Description: this provides point structure.
 ************************************************************/

#include <point.h>
#include <stdint.h>


Point2d* set_point2d(Point2d* p, uint32_t x, uint32_t y) {
    p->x = x;
    p->y = y;

    return p;
}


Point2d* add_point2d(Point2d* p, uint32_t x, uint32_t y) {
    p->x += x;
    p->y += y;

    return p;
}
