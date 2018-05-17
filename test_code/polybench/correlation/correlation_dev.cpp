#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <polybench.h>
#include "correlation.h"
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
  int m = (int) arg2;
  int n = (int) arg3;
  double float_n = *((double *) (&arg4));
  double eps = *((double *) (&arg5));
  double *stddev = (double *) arg6;
  double (*data)[1200] = (double (*)[1200]) arg7;
  double *mean = (double *) arg8;
  int j;
  int i;
#pragma omp parallel for private(i, j)
   for (j = start_index; j < end_index; j++)
    {
      stddev[j] = SCALAR_VAL(0.0);
      for (i = 0; i < _PB_N; i++)
        stddev[j] += (data[i][j] - mean[j]) * (data[i][j] - mean[j]);
      stddev[j] /= float_n;
      stddev[j] = SQRT_FUN(stddev[j]);
      /* The following in an inelegant but usual way to handle
         near-zero std. dev. values, which below would cause a zero-
         divide. */
      stddev[j] = stddev[j] <= eps ? SCALAR_VAL(1.0) : stddev[j];
    }
}