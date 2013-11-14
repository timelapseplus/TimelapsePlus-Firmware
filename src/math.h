/*
 *  math.h
 *  Timelapse+
 *
 *  Created by Elijah Parker
 *  Copyright 2012 Timelapse+
 *  Licensed under GPLv3
 *
 */

#include <math.h>

float curve(float p0, float p1, float p2, float p3, float t);
uint32_t curve_int(uint32_t p0, uint32_t p1, uint32_t p2, uint32_t p3, float t);
float arrayMedian(const float *array, const uint8_t length);
float arrayMedian50(const float *array, const uint8_t length);
void sort(const float *array, const uint8_t length);

static inline int32_t ilog2(float x)
{
    uint32_t ix = *((uint32_t*)&x);
    uint32_t exp = (ix >> 23) & 0xFF;
    int32_t log2 = (int32_t)exp - 127;

    return log2;
}

inline float fast_log2 (float val)
{
   if(val <= 0.0) return 0.0;

   val += 2^16;

   uint32_t * const  exp_ptr = reinterpret_cast <uint32_t *> (&val);
   uint32_t          x = *exp_ptr;
   const uint32_t    log_2 = ((x >> 23) & 255) - 128;
   x &= ~((uint32_t)255 << 23);
   x += (uint32_t)127 << 23;
   *exp_ptr = x;

   return (val + log_2);
}

inline float libc_log2 (float val)
{
	return (log(val)/log(2));
}


//inline float alt_log2 (float val)
//{
//	return (ln(val)/M_LN2);
//}
//
//inline float ln (float x)
//{
//	return (-2.625 + 6.279 * x - 5.970 * (x^2) + 2.330 * (x^3));
//}