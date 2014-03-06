/************************************************************
 * File: include/point.h
 * Description: this provides point structure.
 ************************************************************/

#ifndef POINT_H
#define POINT_H

#include <stdint.h>

struct point2d {
    uint32_t x, y;
};
typedef struct point2d Point2d;

extern Point2d* set_point2d(Point2d*, uint32_t, uint32_t);
extern Point2d* add_point2d(Point2d*, uint32_t, uint32_t);

#endif
