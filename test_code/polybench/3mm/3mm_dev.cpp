#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <polybench.h>
#include "3mm.h"
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
  int ni = (int) arg2;
  int nl = (int) arg3;
  int nj = (int) arg4;
  double (*G)[1100] = (double (*)[1100]) arg5;
  double (*E)[900] = (double (*)[900]) arg6;
  double (*F)[1100] = (double (*)[1100]) arg7;
  int i;
  int j;
  int k;
#pragma omp parallel for private(i, j, k)
  for (i = start_index; i < end_index; i++)
    for (j = 0; j < _PB_NL; j++)
      {
	G[i][j] = SCALAR_VAL(0.0);
	for (k = 0; k < _PB_NJ; ++k)
	  G[i][j] += E[i][k] * F[k][j];
      }
}