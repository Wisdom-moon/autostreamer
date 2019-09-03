#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <polybench.h>
#include "deriche.h"
__kernel void my_kernel
 ( int w,
	int h,
	float a3,
	float a4,
	float b1,
	float b2,
	__global float (*y2)[2160],
	__global float (*imgIn)[2160])
{

  int i;
  float yp1;
  float yp2;
  float xp1;
  float xp2;
  int j;
i = get_global_id(0);


        yp1 = SCALAR_VAL(0.0);
        yp2 = SCALAR_VAL(0.0);
        xp1 = SCALAR_VAL(0.0);
        xp2 = SCALAR_VAL(0.0);
        for (j=_PB_H-1; j>=0; j--) {
            y2[i][j] = a3*xp1 + a4*xp2 + b1*yp1 + b2*yp2;
            xp2 = xp1;
            xp1 = imgIn[i][j];
            yp2 = yp1;
            yp1 = y2[i][j];
        }
    }
}