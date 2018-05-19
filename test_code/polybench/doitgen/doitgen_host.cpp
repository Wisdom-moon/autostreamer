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
/* doitgen.c: this file is part of PolyBench/C */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

/* Include polybench common header. */
#include <polybench.h>

/* Include benchmark-specific header. */
#include "doitgen.h"


/* Array initialization. */
static
void init_array(int nr, int nq, int np,
		DATA_TYPE POLYBENCH_3D(A,NR,NQ,NP,nr,nq,np),
		DATA_TYPE POLYBENCH_2D(C4,NP,NP,np,np))
{
  int i, j, k;

  for (i = 0; i < nr; i++)
    for (j = 0; j < nq; j++)
      for (k = 0; k < np; k++)
	A[i][j][k] = (DATA_TYPE) ((i*j + k)%np) / np;
  for (i = 0; i < np; i++)
    for (j = 0; j < np; j++)
      C4[i][j] = (DATA_TYPE) (i*j % np) / np;
}


/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static
void print_array(int nr, int nq, int np,
		 DATA_TYPE POLYBENCH_3D(A,NR,NQ,NP,nr,nq,np))
{
  int i, j, k;

  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("A");
  for (i = 0; i < nr; i++)
    for (j = 0; j < nq; j++)
      for (k = 0; k < np; k++) {
	if ((i*nq*np+j*np+k) % 20 == 0) fprintf (POLYBENCH_DUMP_TARGET, "\n");
	fprintf (POLYBENCH_DUMP_TARGET, DATA_PRINTF_MODIFIER, A[i][j][k]);
      }
  POLYBENCH_DUMP_END("A");
  POLYBENCH_DUMP_FINISH;
}


/* Main computational kernel. The whole function will be timed,
   including the call and return. */
void kernel_doitgen(int nr, int nq, int np,
		    DATA_TYPE POLYBENCH_3D(A,NR,NQ,NP,nr,nq,np),
		    DATA_TYPE POLYBENCH_2D(C4,NP,NP,np,np),
		    DATA_TYPE POLYBENCH_1D(sum,NP,np))
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

  (hStreams_app_create_buf((double (*)[140][160])A, (((nr)-1)+ 1)* sizeof (double [140][160])));
  (hStreams_app_create_buf((double (*)[160])C4, (((np)-1)+ 1)* sizeof (double [160])));
  (hStreams_app_create_buf((double *)sum, (((np)-1)+ 1)* sizeof (double )));
  int r, q, p, s;

(hStreams_app_xfer_memory((double (*)[140][160])C4, (double (*)[140][160])C4 ,(((np)-1)+ 1)* sizeof (double [160]), 0, HSTR_SRC_TO_SINK, NULL));
(hStreams_app_xfer_memory((double (*)[160])sum, (double (*)[160])sum ,(((np)-1)+ 1)* sizeof (double ), 0, HSTR_SRC_TO_SINK, NULL));
int sub_blocks = (((nr)-1)-0 + 1)/ 4;
int remain_index = (((nr)-1)-0 + 1)% 4;
int start_index = 0;
int end_index = 0;
uint64_t args[8];
args[2] = (uint64_t) nr;
args[3] = (uint64_t) nq;
args[4] = (uint64_t) np;
args[5] = (uint64_t) sum;
args[6] = (uint64_t) A;
args[7] = (uint64_t) C4;
hStreams_ThreadSynchronize();
start_index = 0;
for (int idx_subtask = 0; idx_subtask < 4; idx_subtask++)
{
  args[0] = (uint64_t) start_index;
  end_index = start_index + sub_blocks;
  if (idx_subtask < remain_index)
    end_index ++;
  args[1] = (uint64_t) end_index;
  (hStreams_app_xfer_memory(&A[start_index][0][0], &A[start_index][0][0], (end_index - start_index) * sizeof (double [140][160]), idx_subtask % 2, HSTR_SRC_TO_SINK, NULL));
  (hStreams_EnqueueCompute(
			idx_subtask % 2,
			"kernel",
			5,
			3,
			args,
			NULL,NULL,0));
  (hStreams_app_xfer_memory(&A[start_index][0][0], &A[start_index][0][0], (end_index - start_index) * sizeof (double [140][160]), idx_subtask % 2, HSTR_SINK_TO_SRC, NULL));
  start_index = end_index;
}
hStreams_ThreadSynchronize();
  (hStreams_app_xfer_memory((double **)sum, (double **)sum ,(((np)-1)+ 1)* sizeof (double ), 0, HSTR_SINK_TO_SRC, NULL));

hStreams_app_fini();
}


int main(int argc, char** argv)
{
  /* Retrieve problem size. */
  int nr = NR;
  int nq = NQ;
  int np = NP;

  /* Variable declaration/allocation. */
  POLYBENCH_3D_ARRAY_DECL(A,DATA_TYPE,NR,NQ,NP,nr,nq,np);
  POLYBENCH_1D_ARRAY_DECL(sum,DATA_TYPE,NP,np);
  POLYBENCH_2D_ARRAY_DECL(C4,DATA_TYPE,NP,NP,np,np);

  /* Initialize array(s). */
  init_array (nr, nq, np,
	      POLYBENCH_ARRAY(A),
	      POLYBENCH_ARRAY(C4));

  /* Start timer. */
  polybench_start_instruments;

  /* Run kernel. */
  kernel_doitgen (nr, nq, np,
		  POLYBENCH_ARRAY(A),
		  POLYBENCH_ARRAY(C4),
		  POLYBENCH_ARRAY(sum));

  /* Stop and print timer. */
  polybench_stop_instruments;
  polybench_print_instruments;

  /* Prevent dead-code elimination. All live-out data must be printed
     by the function call in argument. */
  polybench_prevent_dce(print_array(nr, nq, np,  POLYBENCH_ARRAY(A)));

  /* Be clean. */
  POLYBENCH_FREE_ARRAY(A);
  POLYBENCH_FREE_ARRAY(sum);
  POLYBENCH_FREE_ARRAY(C4);

  return 0;
}

