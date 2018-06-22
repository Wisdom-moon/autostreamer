#include <sys/time.h>
#include <string.h>
#include <math.h>
#include <stdio.h> 
#include "model.h"
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
  int doSelf = (int) arg2;
  int i = (int) arg3;
  int n2 = (int) arg4;
  const float xi = *(( float *) (&arg5));
  const float yi = *(( float *) (&arg6));
  const float zi = *(( float *) (&arg7));
  int nbins = (int) arg8;
  struct cartesian *data2 = (struct cartesian *) arg9;
  float *binb = (float *) arg10;
  long long *data_bins = (long long *) arg11;
  int j;
      #pragma omp parallel for      
      for (j = start_index; j < end_index; j++)
        {
	  register float dot = xi * data2[j].x + yi * data2[j].y + 
	    zi * data2[j].z;
	  
	  // run binary search
	  register int min = 0;
	  register int max = nbins;
	  register int k, indx;
	  

	  while (max > min+1)
            {
	      k = (min + max) / 2;
	      if (dot >= binb[k]) 
		max = k;
	      else 
		min = k;
            };
    #pragma omp critical	  
	  if (dot >= binb[min]) 
	    {
//        #pragma omp critical
	      data_bins[min] += 1; /*k = min;*/ 
	    }
	  else if (dot < binb[max]) 
	    { 
  //      #pragma omp critical
	      data_bins[max+1] += 1; /*k = max+1;*/ 
	    }
	  else 
	    { 
    //    #pragma omp critical
	      data_bins[max] += 1; /*k = max;*/ 
	    }
        }
}