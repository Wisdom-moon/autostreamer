#include <hStreams_source.h>
#include <hStreams_app_api.h>
#include <intel-coi/common/COIMacros_common.h>
/**
 * This version is stamped on May 10, 2016
 *
 * Contact:
 *   Louis-Noel Pouchet <pouchet.ohio-state.edu>
 *   Tomofumi Yuki <tomofumi.yuki.fr>
 *
 * Web address: http://polybench.sourceforge.net
 */
/* gesummv.c: this file is part of PolyBench/C */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

/* Include polybench common header. */
#include <polybench.h>

/* Include benchmark-specific header. */
#include "gesummv.h"


/* Array initialization. */
static
void init_array(int n,
		DATA_TYPE *alpha,
		DATA_TYPE *beta,
		DATA_TYPE POLYBENCH_2D(A,N,N,n,n),
		DATA_TYPE POLYBENCH_2D(B,N,N,n,n),
		DATA_TYPE POLYBENCH_1D(x,N,n))
{
  int i, j;

  *alpha = 1.5;
  *beta = 1.2;
  for (i = 0; i < n; i++)
    {
      x[i] = (DATA_TYPE)( i % n) / n;
      for (j = 0; j < n; j++) {
	A[i][j] = (DATA_TYPE) ((i*j+1) % n) / n;
	B[i][j] = (DATA_TYPE) ((i*j+2) % n) / n;
      }
    }
}


/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static
void print_array(int n,
		 DATA_TYPE POLYBENCH_1D(y,N,n))

{
  int i;

  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("y");
  for (i = 0; i < n; i++) {
    if (i % 20 == 0) fprintf (POLYBENCH_DUMP_TARGET, "\n");
    fprintf (POLYBENCH_DUMP_TARGET, DATA_PRINTF_MODIFIER, y[i]);
  }
  POLYBENCH_DUMP_END("y");
  POLYBENCH_DUMP_FINISH;
}


/* Main computational kernel. The whole function will be timed,
   including the call and return. */
static
void kernel_gesummv(int n,
		    DATA_TYPE alpha,
		    DATA_TYPE beta,
		    DATA_TYPE POLYBENCH_2D(A,N,N,n,n),
		    DATA_TYPE POLYBENCH_2D(B,N,N,n,n),
		    DATA_TYPE POLYBENCH_1D(tmp,N,n),
		    DATA_TYPE POLYBENCH_1D(x,N,n),
		    DATA_TYPE POLYBENCH_1D(y,N,n))
{
  uint32_t logical_streams_per_place= 1;
  uint32_t places_per_domain = 2;
  HSTR_OPTIONS hstreams_options;

  hStreams_GetCurrentOptions(&hstreams_options, sizeof(hstreams_options));
  hstreams_options.verbose = 0;
  hstreams_options.phys_domains_limit = 256;
  char *libNames[20] = {NULL,NULL};
  unsigned int libNameCnt = 0;
  libNames[libNameCnt++] = "kernel.so";
  hstreams_options.libNames = libNames;
  hstreams_options.libNameCnt = (uint16_t)libNameCnt;
  hStreams_SetOptions(&hstreams_options);

  int iret = hStreams_app_init(places_per_domain, logical_streams_per_place);
  if( iret != 0 )
  {
    printf("hstreams_app_init failed!\n");
    exit(-1);
  }

  (hStreams_app_create_buf((double (*)[1300])A, (((n)-1)+ 1)* sizeof (double [1300])));
  (hStreams_app_create_buf((double (*)[1300])B, (((n)-1)+ 1)* sizeof (double [1300])));
  (hStreams_app_create_buf((double *)tmp, (((n)-1)+ 1)* sizeof (double )));
  (hStreams_app_create_buf((double *)x, (((n)-1)+ 1)* sizeof (double )));
  (hStreams_app_create_buf((double *)y, (((n)-1)+ 1)* sizeof (double )));
  int i, j;

(hStreams_app_xfer_memory((double *)x, (double *)x ,(((n)-1)+ 1)* sizeof (double ), 0, HSTR_SRC_TO_SINK, NULL));
int sub_blocks = (((n)-1)-0 + 1)/ 4;
int remain_index = (((n)-1)-0 + 1)% 4;
int start_index = 0;
int end_index = 0;
uint64_t args[10];
args[2] = (uint64_t) n;
args[3] = (uint64_t) *((uint64_t *) (&alpha));
args[4] = (uint64_t) *((uint64_t *) (&beta));
args[5] = (uint64_t) tmp;
args[6] = (uint64_t) y;
args[7] = (uint64_t) A;
args[8] = (uint64_t) x;
args[9] = (uint64_t) B;
hStreams_ThreadSynchronize();
start_index = 0;
for (int idx_subtask = 0; idx_subtask < 4; idx_subtask++)
{
  args[0] = (uint64_t) start_index;
  end_index = start_index + sub_blocks;
  if (idx_subtask < remain_index)
    end_index ++;
  args[1] = (uint64_t) end_index;
  (hStreams_app_xfer_memory(&A[start_index][0], &A[start_index][0], (end_index - start_index) * sizeof (double [1300]), idx_subtask % 2, HSTR_SRC_TO_SINK, NULL));
  (hStreams_app_xfer_memory(&B[start_index][0], &B[start_index][0], (end_index - start_index) * sizeof (double [1300]), idx_subtask % 2, HSTR_SRC_TO_SINK, NULL));
  (hStreams_app_xfer_memory(&tmp[start_index], &tmp[start_index], (end_index - start_index) * sizeof (double ), idx_subtask % 2, HSTR_SRC_TO_SINK, NULL));
  (hStreams_app_xfer_memory(&y[start_index], &y[start_index], (end_index - start_index) * sizeof (double ), idx_subtask % 2, HSTR_SRC_TO_SINK, NULL));
  (hStreams_EnqueueCompute(
			idx_subtask % 2,
			"kernel",
			5,
			5,
			args,
			NULL,NULL,0));
  (hStreams_app_xfer_memory(&tmp[start_index], &tmp[start_index], (end_index - start_index) * sizeof (double ), idx_subtask % 2, HSTR_SINK_TO_SRC, NULL));
  (hStreams_app_xfer_memory(&y[start_index], &y[start_index], (end_index - start_index) * sizeof (double ), idx_subtask % 2, HSTR_SINK_TO_SRC, NULL));
  start_index = end_index;
}
hStreams_ThreadSynchronize();

hStreams_app_fini();
}


int main(int argc, char** argv)
{
  /* Retrieve problem size. */
  int n = N;

  /* Variable declaration/allocation. */
  DATA_TYPE alpha;
  DATA_TYPE beta;
  POLYBENCH_2D_ARRAY_DECL(A, DATA_TYPE, N, N, n, n);
  POLYBENCH_2D_ARRAY_DECL(B, DATA_TYPE, N, N, n, n);
  POLYBENCH_1D_ARRAY_DECL(tmp, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL(x, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL(y, DATA_TYPE, N, n);


  /* Initialize array(s). */
  init_array (n, &alpha, &beta,
	      POLYBENCH_ARRAY(A),
	      POLYBENCH_ARRAY(B),
	      POLYBENCH_ARRAY(x));

  /* Start timer. */
  polybench_start_instruments;

  /* Run kernel. */
  kernel_gesummv (n, alpha, beta,
		  POLYBENCH_ARRAY(A),
		  POLYBENCH_ARRAY(B),
		  POLYBENCH_ARRAY(tmp),
		  POLYBENCH_ARRAY(x),
		  POLYBENCH_ARRAY(y));

  /* Stop and print timer. */
  polybench_stop_instruments;
  polybench_print_instruments;

  /* Prevent dead-code elimination. All live-out data must be printed
     by the function call in argument. */
  polybench_prevent_dce(print_array(n, POLYBENCH_ARRAY(y)));

  /* Be clean. */
  POLYBENCH_FREE_ARRAY(A);
  POLYBENCH_FREE_ARRAY(B);
  POLYBENCH_FREE_ARRAY(tmp);
  POLYBENCH_FREE_ARRAY(x);
  POLYBENCH_FREE_ARRAY(y);

  return 0;
}

