#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <polybench.h>
#include "gemver.h"
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
  int n = (int) arg2;
  double alpha = *((double *) (&arg3));
  double *w = (double *) arg4;
  double (*A)[2000] = (double (*)[2000]) arg5;
  double *x = (double *) arg6;
  int i;
  int j;
#pragma omp parallel for private(i, j)
  for (i = start_index; i < end_index; i++)
    for (j = 0; j < _PB_N; j++)
      w[i] = w[i] +  alpha * A[i][j] * x[j];
}