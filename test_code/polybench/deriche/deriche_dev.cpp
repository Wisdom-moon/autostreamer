#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <polybench.h>
#include "deriche.h"
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
  int w = (int) arg2;
  int h = (int) arg3;
  float a3 = *((float *) (&arg4));
  float a4 = *((float *) (&arg5));
  float b1 = *((float *) (&arg6));
  float b2 = *((float *) (&arg7));
  float (*y2)[2160] = (float (*)[2160]) arg8;
  float (*imgIn)[2160] = (float (*)[2160]) arg9;
  int i;
  float yp1;
  float yp2;
  float xp1;
  float xp2;
  int j;
    #pragma omp parallel for private(i, j, yp1, yp2, xp1, xp2, a3, a4, b1, b2)
    for (i= start_index; i< end_index; i++) {
        yp1 = SCALAR_VAL(0.0);
        yp2 = SCALAR_VAL(0.0);
        xp1 = SCALAR_VAL(0.0);
        xp2 = SCALAR_VAL(0.0);
        for (j=_PB_H-1; j>=0; j--) {
            y2[i][j] = a3*xp1 + a4*xp2 + b1*yp1 + b2*yp2;
            xp2 = xp1;
            xp1 = imgIn[i][j];
            yp2 = yp1;
            yp1 = y2[i][j];
        }
    }
}