#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <polybench.h>
#include "2mm.h"
__kernel void my_kernel
 ( int ni,
	int nl,
	double beta,
	int nj,
	__global double (*D)[1200],
	__global double (*tmp)[900],
	__global double (*C)[1200])
{

  int i;
  int j;
  int k;
i = get_global_id(0);


    for (j = 0; j < _PB_NL; j++)
      {
	D[i][j] *= beta;
	for (k = 0; k < _PB_NJ; ++k)
	  D[i][j] += tmp[i][k] * C[k][j];
      }
}