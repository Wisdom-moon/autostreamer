#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <polybench.h>
#include "trisolv.h"
#include <intel-coi/sink/COIPipeline_sink.h>

COINATIVELIBEXPORT void
kernel ( uint64_t arg0,
	uint64_t arg1,
	uint64_t arg2,
	uint64_t arg3,
	uint64_t arg4,
	uint64_t arg5
)
{

  int start_index = (int) arg0;
  int end_index = (int) arg1;
  int n = (int) arg2;
  double *x = (double *) arg3;
  double *b = (double *) arg4;
  double (*L)[2000] = (double (*)[2000]) arg5;
  int i;
  int j;
#pragma omp parallel for private(i, j)
  for (i = start_index; i < end_index; i++)
    {
      x[i] = b[i];
      for (j = 0; j <i; j++)
        x[i] -= L[i][j] * x[j];
      x[i] = x[i] / L[i][i];
    }
}