#include <omp.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <intel-coi/sink/COIPipeline_sink.h>

COINATIVELIBEXPORT void
kernel ( uint64_t arg0,
	uint64_t arg1,
	uint64_t arg2,
	uint64_t arg3,
	uint64_t arg4,
	uint64_t arg5
)
{

  int start_index = (int) arg0;
  int end_index = (int) arg1;
  int inputLength = (int) arg2;
  float *hostOutput = (float *) arg3;
  float *hostInput1 = (float *) arg4;
  float *hostInput2 = (float *) arg5;
#pragma omp parallel for
  for(int i= start_index; i< end_index; i++)
  {
    hostOutput[i] = hostInput1[i] + hostInput2[i];
  }
}