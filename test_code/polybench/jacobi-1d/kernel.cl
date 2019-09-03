#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <polybench.h>
#include "jacobi-1d.h"
__kernel void my_kernel
 ( int n,
	__global double *B,
	__global double *A)
{

  int i;
i = get_global_id(0);


	B[i] = 0.33333 * (A[i-1] + A[i] + A[i + 1]);
}