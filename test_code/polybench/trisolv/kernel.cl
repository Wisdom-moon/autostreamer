#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <polybench.h>
#include "trisolv.h"
__kernel void my_kernel
 ( int n,
	__global double *x,
	__global double *b,
	__global double (*L)[2000])
{

  int i;
  int j;
i = get_global_id(0);


    {
      x[i] = b[i];
      for (j = 0; j <i; j++)
        x[i] -= L[i][j] * x[j];
      x[i] = x[i] / L[i][i];
    }
}