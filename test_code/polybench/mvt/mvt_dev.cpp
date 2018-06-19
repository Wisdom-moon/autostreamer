#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <polybench.h>
#include "mvt.h"
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
  int n = (int) arg2;
  double *x1 = (double *) arg3;
  double (*A)[2000] = (double (*)[2000]) arg4;
  double *y_1 = (double *) arg5;
  double *x2 = (double *) arg6;
  double *y_2 = (double *) arg7;
  int i;
  int j;
#pragma omp parallel for private (i, j)
  for (i = start_index; i < end_index; i++)
    for (j = 0; j < _PB_N; j++) 
    {
      x1[i] = x1[i] + A[i][j] * y_1[j];
      x2[i] = x2[i] + A[j][i] * y_2[j];
    }
}