#include <parboil.h>
#include <stdio.h>
#include <stdlib.h>
#include "file.h"
#include "convert_dataset.h"
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
	uint64_t arg10
)
{

  int start_index = (int) arg0;
  int end_index = (int) arg1;
  int i = (int) arg2;
  int dim = (int) arg3;
  int * h_nzcnt = (int *) arg4;
  int * h_ptr = (int *) arg5;
  int * h_indices = (int *) arg6;
  float * h_data = (float *) arg7;
  float * h_x_vector = (float *) arg8;
  float * h_Ax_vector = (float *) arg9;
  int * h_perm = (int *) arg10;
    #pragma omp parallel for
		for (i = start_index; i < end_index; i++) {
      int k;
		  float sum = 0.0f;
		  //int  bound = h_nzcnt[i / 32];
		  int  bound = h_nzcnt[i];
		  for(k=0;k<bound;k++ ) {
			int j = h_ptr[k] + i;
			int in = h_indices[j];

			float d = h_data[j];
			float t = h_x_vector[in];

			sum += d*t;
		  }
    //  #pragma omp critical 
		  h_Ax_vector[h_perm[i]] = sum;
		}
}