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
/* gemm.c: this file is part of PolyBench/C */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

/* Include polybench common header. */
#include <polybench.h>

/* Include benchmark-specific header. */
#include "gemm.h"


/* Array initialization. */
static
void init_array(int ni, int nj, int nk,
		DATA_TYPE *alpha,
		DATA_TYPE *beta,
		DATA_TYPE POLYBENCH_2D(C,NI,NJ,ni,nj),
		DATA_TYPE POLYBENCH_2D(A,NI,NK,ni,nk),
		DATA_TYPE POLYBENCH_2D(B,NK,NJ,nk,nj))
{
  int i, j;

  *alpha = 1.5;
  *beta = 1.2;
  for (i = 0; i < ni; i++)
    for (j = 0; j < nj; j++)
      C[i][j] = (DATA_TYPE) ((i*j+1) % ni) / ni;
  for (i = 0; i < ni; i++)
    for (j = 0; j < nk; j++)
      A[i][j] = (DATA_TYPE) (i*(j+1) % nk) / nk;
  for (i = 0; i < nk; i++)
    for (j = 0; j < nj; j++)
      B[i][j] = (DATA_TYPE) (i*(j+2) % nj) / nj;
}


/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static
void print_array(int ni, int nj,
		 DATA_TYPE POLYBENCH_2D(C,NI,NJ,ni,nj))
{
  int i, j;

  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("C");
  for (i = 0; i < ni; i++)
    for (j = 0; j < nj; j++) {
	if ((i * ni + j) % 20 == 0) fprintf (POLYBENCH_DUMP_TARGET, "\n");
	fprintf (POLYBENCH_DUMP_TARGET, DATA_PRINTF_MODIFIER, C[i][j]);
    }
  POLYBENCH_DUMP_END("C");
  POLYBENCH_DUMP_FINISH;
}


/* Main computational kernel. The whole function will be timed,
   including the call and return. */
static
void kernel_gemm(int ni, int nj, int nk,
		 DATA_TYPE alpha,
		 DATA_TYPE beta,
		 DATA_TYPE POLYBENCH_2D(C,NI,NJ,ni,nj),
		 DATA_TYPE POLYBENCH_2D(A,NI,NK,ni,nk),
		 DATA_TYPE POLYBENCH_2D(B,NK,NJ,nk,nj))
{
  int i, j, k;

//BLAS PARAMS
//TRANSA = 'N'
//TRANSB = 'N'
// => Form C := alpha*A*B + beta*C,
//A is NIxNK
//B is NKxNJ
//C is NIxNJ
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

(hStreams_app_create_buf((double (*)[1200])A, (((ni)-1)-(0) + 1)* sizeof (double [1200])));
(hStreams_app_create_buf((double (*)[1100])B, (((nk)-1)-(0) + 1)* sizeof (double [1100])));
(hStreams_app_create_buf((double (*)[1100])C, (((ni)-1)-(0) + 1)* sizeof (double [1100])));
(hStreams_app_xfer_memory((double (*)[1200])B, (double (*)[1200])B ,(((nk)-1)-(0) + 1)* sizeof (double [1100]), 0, HSTR_SRC_TO_SINK, NULL));
int sub_blocks = ni/ 4;
int remain_index = ni% 4;
int start_index = 0;
int end_index = 0;
uint64_t args[10];
args[2] = (uint64_t) ni;
args[3] = (uint64_t) nj;
args[4] = (uint64_t) *((uint64_t *) (&beta));
args[5] = (uint64_t) nk;
args[6] = (uint64_t) *((uint64_t *) (&alpha));
args[7] = (uint64_t) C;
args[8] = (uint64_t) A;
args[9] = (uint64_t) B;
hStreams_ThreadSynchronize();
start_index = 0;
for (int i = 0; i < 4; i++)
{
  args[0] = (uint64_t) start_index;
  end_index = start_index + sub_blocks;
  if (i < remain_index)
    end_index ++;
  args[1] = (uint64_t) end_index;
  (hStreams_app_xfer_memory(&A[start_index][0], &A[start_index][0], (end_index - start_index) * sizeof (double [1200]), i % 2, HSTR_SRC_TO_SINK, NULL));
  (hStreams_app_xfer_memory(&C[start_index][0], &C[start_index][0], (end_index - start_index) * sizeof (double [1100]), i % 2, HSTR_SRC_TO_SINK, NULL));
  (hStreams_EnqueueCompute(
			i % 2,
			"kernel",
			7,
			3,
			args,
			NULL,NULL,0));
  (hStreams_app_xfer_memory(&C[start_index][0], &C[start_index][0], (end_index - start_index) * sizeof (double [1100]), i % 2, HSTR_SINK_TO_SRC, NULL));
  start_index = end_index;
}
  hStreams_ThreadSynchronize();
  hStreams_app_fini();

}


int main(int argc, char** argv)
{
  /* Retrieve problem size. */
  int ni = NI;
  int nj = NJ;
  int nk = NK;

  /* Variable declaration/allocation. */
  DATA_TYPE alpha;
  DATA_TYPE beta;
  POLYBENCH_2D_ARRAY_DECL(C,DATA_TYPE,NI,NJ,ni,nj);
  POLYBENCH_2D_ARRAY_DECL(A,DATA_TYPE,NI,NK,ni,nk);
  POLYBENCH_2D_ARRAY_DECL(B,DATA_TYPE,NK,NJ,nk,nj);

  /* Initialize array(s). */
  init_array (ni, nj, nk, &alpha, &beta,
	      POLYBENCH_ARRAY(C),
	      POLYBENCH_ARRAY(A),
	      POLYBENCH_ARRAY(B));

  /* Start timer. */
  polybench_start_instruments;

  /* Run kernel. */
  kernel_gemm (ni, nj, nk,
	       alpha, beta,
	       POLYBENCH_ARRAY(C),
	       POLYBENCH_ARRAY(A),
	       POLYBENCH_ARRAY(B));

  /* Stop and print timer. */
  polybench_stop_instruments;
  polybench_print_instruments;

  /* Prevent dead-code elimination. All live-out data must be printed
     by the function call in argument. */
  polybench_prevent_dce(print_array(ni, nj,  POLYBENCH_ARRAY(C)));

  /* Be clean. */
  POLYBENCH_FREE_ARRAY(C);
  POLYBENCH_FREE_ARRAY(A);
  POLYBENCH_FREE_ARRAY(B);

  return 0;
}

