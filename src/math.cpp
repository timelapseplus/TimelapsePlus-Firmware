/*
 *  math.cpp
 *  Timelapse+
 *
 *  Created by Elijah Parker
 *  Copyright 2012 Timelapse+
 *  Licensed under GPLv3
 *
 */
 
#include "math.h"

/******************************************************************
 *
 *   curve
 * 
 *   Catmull–Rom spline implementation
 *
 ******************************************************************/
 
float curve(float p0, float p1, float p2, float p3, float t)
{
    float t2 = t * t;
    float t3 = t2 * t;

    float ret = (float)
       ((2.0 * p1) +
        (p2 - p0) * t +
        (2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3) * t2 +
        ((3.0 * p1) - p0 - 3.0 * p2 + p3) * t3) / 2.0;

    return ret;
}

/*
int main()
{
  printf("Testing...\n\n");

  float i;

  for(i = 0; i <= 1.1; i += 0.1)
  {
    float y = curve(5, 5, 20, 20, i);
    int j;
    for (j = 0; j < (int) (y * 2.0); j++)
    {
      printf(" ");
    }
    printf("#\n");
//    printf("y = %f   [%f]\n", y, i);
  }

  return 0;
}
*/
