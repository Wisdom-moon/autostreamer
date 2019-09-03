#include "polybench.h"
#include "3mm.h"
__kernel void my_kernel
 ( int ni,
	int nl,
	int nj,
	__global double (*G)[1100],
	__global double (*E)[900],
	__global double (*F)[1100])
{

  int i;
  int j;
  int k;
i = get_global_id(0);


    for (j = 0; j < _PB_NL; j++)
      {
	G[i][j] = SCALAR_VAL(0.0);
	for (k = 0; k < _PB_NJ; ++k)
	  G[i][j] += E[i][k] * F[k][j];
      }
}
