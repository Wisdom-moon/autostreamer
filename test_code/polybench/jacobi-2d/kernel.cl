#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <polybench.h>
#include "jacobi-2d.h"
__kernel void my_kernel
 ( int n,
	__global double (*B)[1300],
	__global double (*A)[1300])
{

  int i;
  int j;
i = get_global_id(0);


	for (j = 1; j < _PB_N - 1; j++)
	  B[i][j] = SCALAR_VAL(0.2) * (A[i][j] + A[i][j-1] + A[i][1+j] + A[1+i][j] + A[i-1][j]);
}