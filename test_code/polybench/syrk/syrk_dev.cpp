#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <polybench.h>
#include "syrk.h"
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
  double beta = *((double *) (&arg3));
  int m = (int) arg4;
  double alpha = *((double *) (&arg5));
  double (*C)[1200] = (double (*)[1200]) arg6;
  double (*A)[1000] = (double (*)[1000]) arg7;
  int i;
  int j;
  int k;
#pragma omp parallel for private (i, j, k)
  for (i = start_index; i < end_index; i++) {
    for (j = 0; j <= i; j++)
      C[i][j] *= beta;
    for (k = 0; k < _PB_M; k++) {
      for (j = 0; j <= i; j++)
        C[i][j] += alpha * A[i][k] * A[j][k];
    }
  }
}