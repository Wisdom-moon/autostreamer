#include "common.h"
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
	uint64_t arg8
)
{

  int start_index = (int) arg0;
  int end_index = (int) arg1;
  const int nx = ( int) arg2;
  const int ny = ( int) arg3;
  const int nz = ( int) arg4;
  float c1 = *((float *) (&arg5));
  float c0 = *((float *) (&arg6));
  float *Anext = (float *) arg7;
  float *A0 = (float *) arg8;
  int i;
  #pragma omp parallel for
	for(i= start_index;i< end_index;i++)
	{
    int j,k;
		for(j=1;j<ny-1;j++)
		{
			for(k=1;k<nz-1;k++)
			{
  //i      #pragma omp critical
				Anext[Index3D (nx, ny, i, j, k)] = 
				(A0[Index3D (nx, ny, i, j, k + 1)] +
				A0[Index3D (nx, ny, i, j, k - 1)] +
				A0[Index3D (nx, ny, i, j + 1, k)] +
				A0[Index3D (nx, ny, i, j - 1, k)] +
				A0[Index3D (nx, ny, i + 1, j, k)] +
				A0[Index3D (nx, ny, i - 1, j, k)])*c1
				- A0[Index3D (nx, ny, i, j, k)]*c0;
			}
		}
	}
}