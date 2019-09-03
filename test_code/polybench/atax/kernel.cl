#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <polybench.h>
#include "atax.h"
__kernel void my_kernel
 ( int m,
	int n,
	__global double *tmp,
	__global double (*A)[2100],
	__global double *x,
	__global double *y)
{

  int i;
  int j;
i = get_global_id(0);


    {
      tmp[i] = SCALAR_VAL(0.0);
      for (j = 0; j < _PB_N; j++)
	tmp[i] = tmp[i] + A[i][j] * x[j];
      for (j = 0; j < _PB_N; j++)
	y[j] = y[j] + A[i][j] * tmp[i];
    }
}