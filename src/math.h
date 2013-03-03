/*
 *  math.h
 *  Timelapse+
 *
 *  Created by Elijah Parker
 *  Copyright 2012 Timelapse+
 *  Licensed under GPLv3
 *
 */

float curve(float p0, float p1, float p2, float p3, float t);
uint32_t curve_int(uint32_t p0, uint32_t p1, uint32_t p2, uint32_t p3, float t);

static inline int32_t ilog2(float x)
{
    uint32_t ix = *((uint32_t*)&x);
    uint32_t exp = (ix >> 23) & 0xFF;
    int32_t log2 = (int32_t)exp - 127;

    return log2;
}

