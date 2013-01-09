/*
 *  math.cpp
 *  Timelapse+
 *
 *  Created by Elijah Parker
 *  Copyright 2012 Timelapse+
 *  Licensed under GPLv3
 *
 */
 
//#include <stdio.h>

#include "math.h"

/******************************************************************
 *
 *   curve
 * 
 *   Catmull–Rom spline implementation
 *
 ******************************************************************/

double curve(double p0, double p1, double p2, double p3, double t)
{
    double t2 = t * t;
    double t3 = t2 * t;

    double ret = (double)
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

  double i;

  for(i = 0; i <= 1.1; i += 0.1)
  {
    double y = curve(5, 5, 20, 20, i);
    int j;
    for (j = 0; j < (int) (y * 2.0); j++)
    {
      printf(" ");
    }
    printf("#\n");
//    printf("y = %f   [%f]\n", y, i);
  }
  for(i = 0; i <= 1.1; i += 0.1)
  {
    double y = curve(5, 5, 20, 20, i);
    printf("y = %f   [%f]\n", y, i);
  }


  return 0;
}
*/

//=((2.0 * C2) + (D2 - B2) * F2 + (2.0 * B2 - 5.0 * C2 + 4.0 * D2 - E2) * (F2*F2) + ((3.0 * C2) - B2 - 3.0 * D2 + E2) * (F2*F2*F2)) / 2.0