#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <polybench.h>
#include "gemver.h"
__kernel void my_kernel
 ( int n,
	double alpha,
	__global double *w,
	__global double (*A)[2000],
	__global double *x)
{

  int i;
  int j;
i = get_global_id(0);


    for (j = 0; j < _PB_N; j++)
      w[i] = w[i] +  alpha * A[i][j] * x[j];
}