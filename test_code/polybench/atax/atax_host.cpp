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
/* atax.c: this file is part of PolyBench/C */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

/* Include polybench common header. */
#include <polybench.h>

/* Include benchmark-specific header. */
#include "atax.h"


/* Array initialization. */
static
void init_array (int m, int n,
		 DATA_TYPE POLYBENCH_2D(A,M,N,m,n),
		 DATA_TYPE POLYBENCH_1D(x,N,n))
{
  int i, j;
  DATA_TYPE fn;
  fn = (DATA_TYPE)n;

  for (i = 0; i < n; i++)
      x[i] = 1 + (i / fn);
  for (i = 0; i < m; i++)
    for (j = 0; j < n; j++)
      A[i][j] = (DATA_TYPE) ((i+j) % n) / (5*m);
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
void kernel_atax(int m, int n,
		 DATA_TYPE POLYBENCH_2D(A,M,N,m,n),
		 DATA_TYPE POLYBENCH_1D(x,N,n),
		 DATA_TYPE POLYBENCH_1D(y,N,n),
		 DATA_TYPE POLYBENCH_1D(tmp,M,m))
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

  (hStreams_app_create_buf((double (*)[2100])A, (((m)-1)+ 1)* sizeof (double [2100])));
  (hStreams_app_create_buf((double *)tmp, (((m)-1)+ 1)* sizeof (double )));
  (hStreams_app_create_buf((double *)x, (((n)-1)+ 1)* sizeof (double )));
  (hStreams_app_create_buf((double *)y, (((n)-1)+ 1)* sizeof (double )));
  int i, j;

  for (i = 0; i < _PB_N; i++)
    y[i] = 0;
(hStreams_app_xfer_memory((double *)x, (double *)x ,(((n)-1)+ 1)* sizeof (double ), 0, HSTR_SRC_TO_SINK, NULL));
(hStreams_app_xfer_memory((double *)y, (double *)y ,(((n)-1)+ 1)* sizeof (double ), 0, HSTR_SRC_TO_SINK, NULL));
int sub_blocks = (((m)-1)-0 + 1)/ 4;
int remain_index = (((m)-1)-0 + 1)% 4;
int start_index = 0;
int end_index = 0;
uint64_t args[8];
args[2] = (uint64_t) m;
args[3] = (uint64_t) n;
args[4] = (uint64_t) tmp;
args[5] = (uint64_t) A;
args[6] = (uint64_t) x;
args[7] = (uint64_t) y;
hStreams_ThreadSynchronize();
start_index = 0;
for (int idx_subtask = 0; idx_subtask < 4; idx_subtask++)
{
  args[0] = (uint64_t) start_index;
  end_index = start_index + sub_blocks;
  if (idx_subtask < remain_index)
    end_index ++;
  args[1] = (uint64_t) end_index;
  (hStreams_app_xfer_memory(&A[start_index][0], &A[start_index][0], (end_index - start_index) * sizeof (double [2100]), idx_subtask % 2, HSTR_SRC_TO_SINK, NULL));
  (hStreams_app_xfer_memory(&tmp[start_index], &tmp[start_index], (end_index - start_index) * sizeof (double ), idx_subtask % 2, HSTR_SRC_TO_SINK, NULL));
  (hStreams_EnqueueCompute(
			idx_subtask % 2,
			"kernel",
			4,
			4,
			args,
			NULL,NULL,0));
  (hStreams_app_xfer_memory(&tmp[start_index], &tmp[start_index], (end_index - start_index) * sizeof (double ), idx_subtask % 2, HSTR_SINK_TO_SRC, NULL));
  start_index = end_index;
}
hStreams_ThreadSynchronize();
  (hStreams_app_xfer_memory((double *)y, (double *)y ,(((n)-1)+ 1)* sizeof (double ), 0, HSTR_SINK_TO_SRC, NULL));

hStreams_app_fini();
}


int main(int argc, char** argv)
{
  /* Retrieve problem size. */
  int m = M;
  int n = N;

  /* Variable declaration/allocation. */
  POLYBENCH_2D_ARRAY_DECL(A, DATA_TYPE, M, N, m, n);
  POLYBENCH_1D_ARRAY_DECL(x, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL(y, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL(tmp, DATA_TYPE, M, m);

  /* Initialize array(s). */
  init_array (m, n, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(x));

  /* Start timer. */
  polybench_start_instruments;

  /* Run kernel. */
  kernel_atax (m, n,
	       POLYBENCH_ARRAY(A),
	       POLYBENCH_ARRAY(x),
	       POLYBENCH_ARRAY(y),
	       POLYBENCH_ARRAY(tmp));

  /* Stop and print timer. */
  polybench_stop_instruments;
  polybench_print_instruments;

  /* Prevent dead-code elimination. All live-out data must be printed
     by the function call in argument. */
  polybench_prevent_dce(print_array(n, POLYBENCH_ARRAY(y)));

  /* Be clean. */
  POLYBENCH_FREE_ARRAY(A);
  POLYBENCH_FREE_ARRAY(x);
  POLYBENCH_FREE_ARRAY(y);
  POLYBENCH_FREE_ARRAY(tmp);

  return 0;
}

