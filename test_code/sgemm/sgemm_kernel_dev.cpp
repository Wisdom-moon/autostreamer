#include <iostream>
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
	uint64_t arg11,
	uint64_t arg12
)
{

  int start_index = (int) arg0;
  int end_index = (int) arg1;
  int m = (int) arg2;
  int n = (int) arg3;
  int k = (int) arg4;
  int lda = (int) arg5;
  int ldb = (int) arg6;
  int ldc = (int) arg7;
  float beta = (float) arg8;
  float alpha = (float) arg9;
  const float * A = (const float *) arg10;
  const float * B = (const float *) arg11;
  float * C = (float *) arg12;
  #pragma omp parallel for collapse (2)
  for (int mm = start_index; mm < end_index; ++mm) {
    for (int nn = 0; nn < n; ++nn) {
      float c = 0.0f;
      for (int i = 0; i < k; ++i) {
        float a = A[mm + i * lda]; 
        float b = B[nn + i * ldb];
        c += a * b;
      }
      C[mm+nn*ldc] = C[mm+nn*ldc] * beta + alpha * c;
    }
  }
}