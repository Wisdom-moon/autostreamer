#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <polybench.h>
#include "covariance.h"
#include <intel-coi/sink/COIPipeline_sink.h>

COINATIVELIBEXPORT void
kernel ( uint64_t arg0,
	uint64_t arg1,
	uint64_t arg2,
	uint64_t arg3,
	uint64_t arg4,
	uint64_t arg5,
	uint64_t arg6
)
{

  int start_index = (int) arg0;
  int end_index = (int) arg1;
  int m = (int) arg2;
  int n = (int) arg3;
  double float_n = *((double *) (&arg4));
  double (*cov)[1200] = (double (*)[1200]) arg5;
  double (*data)[1200] = (double (*)[1200]) arg6;
  int i;
  int j;
  int k;
#pragma omp parallel for private(i, j, k)
  for (i = start_index; i < end_index; i++)
    for (j = i; j < _PB_M; j++)
      {
        cov[i][j] = SCALAR_VAL(0.0);
        for (k = 0; k < _PB_N; k++)
	  cov[i][j] += data[k][i] * data[k][j];
        cov[i][j] /= (float_n - SCALAR_VAL(1.0));
        cov[j][i] = cov[i][j];
      }
}