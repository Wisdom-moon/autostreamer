#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <polybench.h>
#include "adi.h"
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
	uint64_t arg9,
	uint64_t arg10,
	uint64_t arg11
)
{

  int start_index = (int) arg0;
  int end_index = (int) arg1;
  int n = (int) arg2;
  double c = *((double *) (&arg3));
  double a = *((double *) (&arg4));
  double b = *((double *) (&arg5));
  double d = *((double *) (&arg6));
  double f = *((double *) (&arg7));
  double (*v)[1000] = (double (*)[1000]) arg8;
  double (*p)[1000] = (double (*)[1000]) arg9;
  double (*q)[1000] = (double (*)[1000]) arg10;
  double (*u)[1000] = (double (*)[1000]) arg11;
  int i;
  int j;
#pragma omp parallel for private (i, j, c, d, b, a)
    for (i= start_index; i< end_index; i++) {
      v[0][i] = SCALAR_VAL(1.0);
      p[i][0] = SCALAR_VAL(0.0);
      q[i][0] = v[0][i];
      for (j=1; j<_PB_N-1; j++) {
        p[i][j] = -c / (a*p[i][j-1]+b);
        q[i][j] = (-d*u[j][i-1]+(SCALAR_VAL(1.0)+SCALAR_VAL(2.0)*d)*u[j][i] - f*u[j][i+1]-a*q[i][j-1])/(a*p[i][j-1]+b);
      }

      v[_PB_N-1][i] = SCALAR_VAL(1.0);
      for (j=_PB_N-2; j>=1; j--) {
        v[j][i] = p[i][j] * v[j+1][i] + q[i][j];
      }
    }
}