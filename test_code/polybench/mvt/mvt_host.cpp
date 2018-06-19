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
/* mvt.c: this file is part of PolyBench/C */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

/* Include polybench common header. */
#include <polybench.h>

/* Include benchmark-specific header. */
#include "mvt.h"


/* Array initialization. */
static
void init_array(int n,
		DATA_TYPE POLYBENCH_1D(x1,N,n),
		DATA_TYPE POLYBENCH_1D(x2,N,n),
		DATA_TYPE POLYBENCH_1D(y_1,N,n),
		DATA_TYPE POLYBENCH_1D(y_2,N,n),
		DATA_TYPE POLYBENCH_2D(A,N,N,n,n))
{
  int i, j;

  for (i = 0; i < n; i++)
    {
      x1[i] = (DATA_TYPE) (i % n) / n;
      x2[i] = (DATA_TYPE) ((i + 1) % n) / n;
      y_1[i] = (DATA_TYPE) ((i + 3) % n) / n;
      y_2[i] = (DATA_TYPE) ((i + 4) % n) / n;
      for (j = 0; j < n; j++)
	A[i][j] = (DATA_TYPE) (i*j % n) / n;
    }
}


/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static
void print_array(int n,
		 DATA_TYPE POLYBENCH_1D(x1,N,n),
		 DATA_TYPE POLYBENCH_1D(x2,N,n))

{
  int i;

  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("x1");
  for (i = 0; i < n; i++) {
    if (i % 20 == 0) fprintf (POLYBENCH_DUMP_TARGET, "\n");
    fprintf (POLYBENCH_DUMP_TARGET, DATA_PRINTF_MODIFIER, x1[i]);
  }
  POLYBENCH_DUMP_END("x1");

  POLYBENCH_DUMP_BEGIN("x2");
  for (i = 0; i < n; i++) {
    if (i % 20 == 0) fprintf (POLYBENCH_DUMP_TARGET, "\n");
    fprintf (POLYBENCH_DUMP_TARGET, DATA_PRINTF_MODIFIER, x2[i]);
  }
  POLYBENCH_DUMP_END("x2");
  POLYBENCH_DUMP_FINISH;
}


/* Main computational kernel. The whole function will be timed,
   including the call and return. */
static
void kernel_mvt(int n,
		DATA_TYPE POLYBENCH_1D(x1,N,n),
		DATA_TYPE POLYBENCH_1D(x2,N,n),
		DATA_TYPE POLYBENCH_1D(y_1,N,n),
		DATA_TYPE POLYBENCH_1D(y_2,N,n),
		DATA_TYPE POLYBENCH_2D(A,N,N,n,n))
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

  (hStreams_app_create_buf((double (*)[2000])A, (((n)-1)+ 1)* sizeof (double [2000])));
  (hStreams_app_create_buf((double *)x1, (((n)-1)+ 1)* sizeof (double )));
  (hStreams_app_create_buf((double *)x2, (((n)-1)+ 1)* sizeof (double )));
  (hStreams_app_create_buf((double *)y_1, (((n)-1)+ 1)* sizeof (double )));
  (hStreams_app_create_buf((double *)y_2, (((n)-1)+ 1)* sizeof (double )));
  int i, j;

(hStreams_app_xfer_memory((double (*)[2000])A, (double (*)[2000])A ,(((n)-1)+ 1)* sizeof (double [2000]), 0, HSTR_SRC_TO_SINK, NULL));
(hStreams_app_xfer_memory((double *)y_1, (double *)y_1 ,(((n)-1)+ 1)* sizeof (double ), 0, HSTR_SRC_TO_SINK, NULL));
(hStreams_app_xfer_memory((double *)y_2, (double *)y_2 ,(((n)-1)+ 1)* sizeof (double ), 0, HSTR_SRC_TO_SINK, NULL));
int sub_blocks = (((n)-1)-0 + 1)/ 4;
int remain_index = (((n)-1)-0 + 1)% 4;
int start_index = 0;
int end_index = 0;
uint64_t args[8];
args[2] = (uint64_t) n;
args[3] = (uint64_t) x1;
args[4] = (uint64_t) A;
args[5] = (uint64_t) y_1;
args[6] = (uint64_t) x2;
args[7] = (uint64_t) y_2;
hStreams_ThreadSynchronize();
start_index = 0;
for (int idx_subtask = 0; idx_subtask < 4; idx_subtask++)
{
  args[0] = (uint64_t) start_index;
  end_index = start_index + sub_blocks;
  if (idx_subtask < remain_index)
    end_index ++;
  args[1] = (uint64_t) end_index;
  (hStreams_app_xfer_memory(&x1[start_index], &x1[start_index], (end_index - start_index) * sizeof (double ), idx_subtask % 2, HSTR_SRC_TO_SINK, NULL));
  (hStreams_app_xfer_memory(&x2[start_index], &x2[start_index], (end_index - start_index) * sizeof (double ), idx_subtask % 2, HSTR_SRC_TO_SINK, NULL));
  (hStreams_EnqueueCompute(
			idx_subtask % 2,
			"kernel",
			3,
			5,
			args,
			NULL,NULL,0));
  (hStreams_app_xfer_memory(&x1[start_index], &x1[start_index], (end_index - start_index) * sizeof (double ), idx_subtask % 2, HSTR_SINK_TO_SRC, NULL));
  (hStreams_app_xfer_memory(&x2[start_index], &x2[start_index], (end_index - start_index) * sizeof (double ), idx_subtask % 2, HSTR_SINK_TO_SRC, NULL));
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
  POLYBENCH_2D_ARRAY_DECL(A, DATA_TYPE, N, N, n, n);
  POLYBENCH_1D_ARRAY_DECL(x1, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL(x2, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL(y_1, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL(y_2, DATA_TYPE, N, n);


  /* Initialize array(s). */
  init_array (n,
	      POLYBENCH_ARRAY(x1),
	      POLYBENCH_ARRAY(x2),
	      POLYBENCH_ARRAY(y_1),
	      POLYBENCH_ARRAY(y_2),
	      POLYBENCH_ARRAY(A));

  /* Start timer. */
  polybench_start_instruments;

  /* Run kernel. */
  kernel_mvt (n,
	      POLYBENCH_ARRAY(x1),
	      POLYBENCH_ARRAY(x2),
	      POLYBENCH_ARRAY(y_1),
	      POLYBENCH_ARRAY(y_2),
	      POLYBENCH_ARRAY(A));

  /* Stop and print timer. */
  polybench_stop_instruments;
  polybench_print_instruments;

  /* Prevent dead-code elimination. All live-out data must be printed
     by the function call in argument. */
  polybench_prevent_dce(print_array(n, POLYBENCH_ARRAY(x1), POLYBENCH_ARRAY(x2)));

  /* Be clean. */
  POLYBENCH_FREE_ARRAY(A);
  POLYBENCH_FREE_ARRAY(x1);
  POLYBENCH_FREE_ARRAY(x2);
  POLYBENCH_FREE_ARRAY(y_1);
  POLYBENCH_FREE_ARRAY(y_2);

  return 0;
}

