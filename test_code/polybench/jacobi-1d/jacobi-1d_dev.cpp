#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <polybench.h>
#include "jacobi-1d.h"
#include <intel-coi/sink/COIPipeline_sink.h>

COINATIVELIBEXPORT void
kernel ( uint64_t arg0,
	uint64_t arg1,
	uint64_t arg2,
	uint64_t arg3,
	uint64_t arg4
)
{

  int start_index = (int) arg0;
  int end_index = (int) arg1;
  int n = (int) arg2;
  double *B = (double *) arg3;
  double *A = (double *) arg4;
  int i;
#pragma omp parallel for private(i)
      for (i = start_index; i < end_index; i++)
	B[i] = 0.33333 * (A[i-1] + A[i] + A[i + 1]);
}