#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <polybench.h>
#include "syrk.h"
__kernel void my_kernel
 ( int n,
	double beta,
	int m,
	double alpha,
	__global double (*C)[1200],
	__global double (*A)[1000])
{

  int i;
  int j;
  int k;
i = get_global_id(0);


    for (j = 0; j <= i; j++)
      C[i][j] *= beta;
    for (k = 0; k < _PB_M; k++) {
      for (j = 0; j <= i; j++)
        C[i][j] += alpha * A[i][k] * A[j][k];
    }
  }
}