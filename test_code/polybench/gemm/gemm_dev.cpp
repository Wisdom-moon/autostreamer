#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <polybench.h>
#include "gemm.h"
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
  int ni = (int) arg2;
  int nj = (int) arg3;
  double beta = *((double *) (&arg4));
  int nk = (int) arg5;
  double alpha = *((double *) (&arg6));
  double (*C)[1100] = (double (*)[1100]) arg7;
  double (*A)[1200] = (double (*)[1200]) arg8;
  double (*B)[1100] = (double (*)[1100]) arg9;
  int i;
  int j;
  int k;
#pragma omp parallel for private(i, j, k)
  for (i = start_index; i < end_index; i++) {
    for (j = 0; j < _PB_NJ; j++)
	C[i][j] *= beta;
    for (k = 0; k < _PB_NK; k++) {
       for (j = 0; j < _PB_NJ; j++)
	  C[i][j] += alpha * A[i][k] * B[k][j];
    }
  }
}