#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <polybench.h>
#include "ludcmp.h"
__kernel void my_kernel
 ( int n,
	__global double *b,
	__global double (*A)[2000],
	__global double *y)
{

  int i;
  double w;
  int j;
i = get_global_id(0);


     w = b[i];
     for (j = 0; j < i; j++)
        w -= A[i][j] * y[j];
     y[i] = w;
  }
}