#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <polybench.h>
#include "doitgen.h"
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
  int nr = (int) arg2;
  int nq = (int) arg3;
  int np = (int) arg4;
  double *sum = (double *) arg5;
  double (*A)[140][160] = (double (*)[140][160]) arg6;
  double (*C4)[160] = (double (*)[160]) arg7;
  int r;
  int q;
  int p;
  int s;
#pragma omp parallel for private(r, q, p, s)
  for (r = start_index; r < end_index; r++)
    for (q = 0; q < _PB_NQ; q++)  {
      for (p = 0; p < _PB_NP; p++)  {
	sum[p] = SCALAR_VAL(0.0);
	for (s = 0; s < _PB_NP; s++)
	  sum[p] += A[r][q][s] * C4[s][p];
      }
      for (p = 0; p < _PB_NP; p++)
	A[r][q][p] = sum[p];
    }
}