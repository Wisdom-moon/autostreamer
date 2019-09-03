#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <polybench.h>
#include "gesummv.h"
__kernel void my_kernel
 ( int n,
	double alpha,
	double beta,
	__global double *tmp,
	__global double *y,
	__global double (*A)[1300],
	__global double *x,
	__global double (*B)[1300])
{

  int i;
  int j;
i = get_global_id(0);


    {
      tmp[i] = SCALAR_VAL(0.0);
      y[i] = SCALAR_VAL(0.0);
      for (j = 0; j < _PB_N; j++)
	{
	  tmp[i] = A[i][j] * x[j] + tmp[i];
	  y[i] = B[i][j] * x[j] + y[i];
	}
      y[i] = alpha * tmp[i] + beta * y[i];
    }
}