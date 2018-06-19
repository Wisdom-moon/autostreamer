#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <polybench.h>
#include "2mm.h"
#include <intel-coi/sink/COIPipeline_sink.h>

COINATIVELIBEXPORT void
kernel ( uint64_t arg0,
	uint64_t arg1,
	uint64_t arg2,
	uint64_t arg3,
	uint64_t arg4,
	uint64_t arg5,
	uint64_t arg6,
	uint64_t arg7,
	uint64_t arg8
)
{

  int start_index = (int) arg0;
  int end_index = (int) arg1;
  int ni = (int) arg2;
  int nl = (int) arg3;
  double beta = *((double *) (&arg4));
  int nj = (int) arg5;
  double (*D)[1200] = (double (*)[1200]) arg6;
  double (*tmp)[900] = (double (*)[900]) arg7;
  double (*C)[1200] = (double (*)[1200]) arg8;
  int i;
  int j;
  int k;
#pragma omp parallel for private(i, j, beta)
  for (i = start_index; i < end_index; i++)
    for (j = 0; j < _PB_NL; j++)
      {
	D[i][j] *= beta;
	for (k = 0; k < _PB_NJ; ++k)
	  D[i][j] += tmp[i][k] * C[k][j];
      }
}