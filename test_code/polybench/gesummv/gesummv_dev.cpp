#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <polybench.h>
#include "gesummv.h"
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
	uint64_t arg8,
	uint64_t arg9
)
{

  int start_index = (int) arg0;
  int end_index = (int) arg1;
  int n = (int) arg2;
  double alpha = *((double *) (&arg3));
  double beta = *((double *) (&arg4));
  double *tmp = (double *) arg5;
  double *y = (double *) arg6;
  double (*A)[1300] = (double (*)[1300]) arg7;
  double *x = (double *) arg8;
  double (*B)[1300] = (double (*)[1300]) arg9;
  int i;
  int j;
#pragma omp parallel for private (i, j)
  for (i = start_index; i < end_index; i++)
    {
      tmp[i] = SCALAR_VAL(0.0);
      y[i] = SCALAR_VAL(0.0);
      for (j = 0; j < _PB_N; j++)
	{
	  tmp[i] = A[i][j] * x[j] + tmp[i];
	  y[i] = B[i][j] * x[j] + y[i];
	}
      y[i] = alpha * tmp[i] + beta * y[i];
    }
}