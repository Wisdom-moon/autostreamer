#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <polybench.h>
#include "atax.h"
#include <intel-coi/sink/COIPipeline_sink.h>

COINATIVELIBEXPORT void
kernel ( uint64_t arg0,
	uint64_t arg1,
	uint64_t arg2,
	uint64_t arg3,
	uint64_t arg4,
	uint64_t arg5,
	uint64_t arg6,
	uint64_t arg7
)
{

  int start_index = (int) arg0;
  int end_index = (int) arg1;
  int m = (int) arg2;
  int n = (int) arg3;
  double *tmp = (double *) arg4;
  double (*A)[2100] = (double (*)[2100]) arg5;
  double *x = (double *) arg6;
  double *y = (double *) arg7;
  int i;
  int j;
#pragma omp parallel for private(i, j)
  for (i = start_index; i < end_index; i++)
    {
      tmp[i] = SCALAR_VAL(0.0);
      for (j = 0; j < _PB_N; j++)
	tmp[i] = tmp[i] + A[i][j] * x[j];
      for (j = 0; j < _PB_N; j++)
	y[j] = y[j] + A[i][j] * tmp[i];
    }
}