#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <polybench.h>
#include "correlation.h"
__kernel void my_kernel
 ( int m,
	int n,
	double float_n,
	double eps,
	__global double *stddev,
	__global double (*data)[1200],
	__global double *mean)
{

  int j;
  int i;
j = get_global_id(0);


    {
      stddev[j] = SCALAR_VAL(0.0);
      for (i = 0; i < _PB_N; i++)
        stddev[j] += (data[i][j] - mean[j]) * (data[i][j] - mean[j]);
      stddev[j] /= float_n;
      stddev[j] = SQRT_FUN(stddev[j]);
      /* The following in an inelegant but usual way to handle
         near-zero std. dev. values, which below would cause a zero-
         divide. */
      stddev[j] = stddev[j] <= eps ? SCALAR_VAL(1.0) : stddev[j];
    }
}