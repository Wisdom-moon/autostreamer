#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <polybench.h>
#include "gemm.h"
__kernel void my_kernel
 ( int ni,
	int nj,
	double beta,
	int nk,
	double alpha,
	__global double (*C)[1100],
	__global double (*A)[1200],
	__global double (*B)[1100])
{

  int i;
  int j;
  int k;
i = get_global_id(0);


    for (j = 0; j < _PB_NJ; j++)
	C[i][j] *= beta;
    for (k = 0; k < _PB_NK; k++) {
       for (j = 0; j < _PB_NJ; j++)
	  C[i][j] += alpha * A[i][k] * B[k][j];
    }
  }
}