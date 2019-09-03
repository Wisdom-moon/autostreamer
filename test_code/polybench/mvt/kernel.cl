#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <polybench.h>
#include "mvt.h"
__kernel void my_kernel
 ( int n,
	__global double *x1,
	__global double (*A)[2000],
	__global double *y_1,
	__global double *x2,
	__global double *y_2)
{

  int i;
  int j;
i = get_global_id(0);


    for (j = 0; j < _PB_N; j++) 
    {
      x1[i] = x1[i] + A[i][j] * y_1[j];
      x2[i] = x2[i] + A[j][i] * y_2[j];
    }
}