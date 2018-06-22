#include <math.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
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
  int numK = (int) arg2;
  int numX = (int) arg3;
  struct kValues *kVals = (struct kValues *) arg4;
  float *x = (float *) arg5;
  float *y = (float *) arg6;
  float *z = (float *) arg7;
  float *Qr = (float *) arg8;
  float *Qi = (float *) arg9;
  int indexK;
  int indexX;
  float expArg;
  float cosArg;
  float sinArg;
  #pragma omp parallel for
  for (indexK = start_index; indexK < end_index; indexK++) {
    for (indexX = 0; indexX < numX; indexX++) {
      expArg = PIx2 * (kVals[indexK].Kx * x[indexX] +
                       kVals[indexK].Ky * y[indexX] +
                       kVals[indexK].Kz * z[indexX]);

      cosArg = cosf(expArg);
      sinArg = sinf(expArg);

      float phi = kVals[indexK].PhiMag;
      Qr[indexX] += phi * cosArg;
      Qi[indexX] += phi * sinArg;
    }
  }
}