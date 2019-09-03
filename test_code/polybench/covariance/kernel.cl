#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <polybench.h>
#include "covariance.h"
__kernel void my_kernel
 ( int m,
	int n,
	double float_n,
	__global double (*cov)[1200],
	__global double (*data)[1200])
{

  int i;
  int j;
  int k;
i = get_global_id(0);


    for (j = i; j < _PB_M; j++)
      {
        cov[i][j] = SCALAR_VAL(0.0);
        for (k = 0; k < _PB_N; k++)
	  cov[i][j] += data[k][i] * data[k][j];
        cov[i][j] /= (float_n - SCALAR_VAL(1.0));
        cov[j][i] = cov[i][j];
      }
}