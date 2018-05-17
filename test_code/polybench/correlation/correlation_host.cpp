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
/* correlation.c: this file is part of PolyBench/C */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

/* Include polybench common header. */
#include <polybench.h>

/* Include benchmark-specific header. */
#include "correlation.h"


/* Array initialization. */
static
void init_array (int m,
		 int n,
		 DATA_TYPE *float_n,
		 DATA_TYPE POLYBENCH_2D(data,N,M,n,m))
{
  int i, j;

  *float_n = (DATA_TYPE)N;

  for (i = 0; i < N; i++)
    for (j = 0; j < M; j++)
      data[i][j] = (DATA_TYPE)(i*j)/M + i;

}


/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static
void print_array(int m,
		 DATA_TYPE POLYBENCH_2D(corr,M,M,m,m))

{
  int i, j;

  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("corr");
  for (i = 0; i < m; i++)
    for (j = 0; j < m; j++) {
      if ((i * m + j) % 20 == 0) fprintf (POLYBENCH_DUMP_TARGET, "\n");
      fprintf (POLYBENCH_DUMP_TARGET, DATA_PRINTF_MODIFIER, corr[i][j]);
    }
  POLYBENCH_DUMP_END("corr");
  POLYBENCH_DUMP_FINISH;
}


/* Main computational kernel. The whole function will be timed,
   including the call and return. */
static
void kernel_correlation(int m, int n,
			DATA_TYPE float_n,
			DATA_TYPE POLYBENCH_2D(data,N,M,n,m),
			DATA_TYPE POLYBENCH_2D(corr,M,M,m,m),
			DATA_TYPE POLYBENCH_1D(mean,M,m),
			DATA_TYPE POLYBENCH_1D(stddev,M,m))
{
  int i, j, k;

  DATA_TYPE eps = SCALAR_VAL(0.1);


  for (j = 0; j < _PB_M; j++)
    {
      mean[j] = SCALAR_VAL(0.0);
      for (i = 0; i < _PB_N; i++)
	mean[j] += data[i][j];
      mean[j] /= float_n;
    }


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

(hStreams_app_create_buf((double (*)[1200])data, (((n)-1)-(0) + 1)* sizeof (double [1200])));
(hStreams_app_create_buf((double *)mean, (((m)-1)-(0) + 1)* sizeof (double )));
(hStreams_app_create_buf((double *)stddev, (((m)-1)-(0) + 1)* sizeof (double )));
(hStreams_app_xfer_memory((double (*)[1200])data, (double (*)[1200])data ,(((n)-1)-(0) + 1)* sizeof (double [1200]), 0, HSTR_SRC_TO_SINK, NULL));
int sub_blocks = m/ 4;
int remain_index = m% 4;
int start_index = 0;
int end_index = 0;
uint64_t args[9];
args[2] = (uint64_t) m;
args[3] = (uint64_t) n;
args[4] = (uint64_t) *((uint64_t *) (&float_n));
args[5] = (uint64_t) *((uint64_t *) (&eps));
args[6] = (uint64_t) stddev;
args[7] = (uint64_t) data;
args[8] = (uint64_t) mean;
hStreams_ThreadSynchronize();
start_index = 0;
for (int i = 0; i < 4; i++)
{
  args[0] = (uint64_t) start_index;
  end_index = start_index + sub_blocks;
  if (i < remain_index)
    end_index ++;
  args[1] = (uint64_t) end_index;
  (hStreams_app_xfer_memory(&mean[start_index], &mean[start_index], (end_index - start_index) * sizeof (double ), i % 2, HSTR_SRC_TO_SINK, NULL));
  (hStreams_app_xfer_memory(&stddev[start_index], &stddev[start_index], (end_index - start_index) * sizeof (double ), i % 2, HSTR_SRC_TO_SINK, NULL));
  (hStreams_EnqueueCompute(
			i % 2,
			"kernel",
			6,
			3,
			args,
			NULL,NULL,0));
  (hStreams_app_xfer_memory(&stddev[start_index], &stddev[start_index], (end_index - start_index) * sizeof (double ), i % 2, HSTR_SINK_TO_SRC, NULL));
  start_index = end_index;
}
    hStreams_ThreadSynchronize();
    hStreams_app_fini();

  /* Center and reduce the column vectors. */
  for (i = 0; i < _PB_N; i++)
    for (j = 0; j < _PB_M; j++)
      {
        data[i][j] -= mean[j];
        data[i][j] /= SQRT_FUN(float_n) * stddev[j];
      }

  /* Calculate the m * m correlation matrix. */
  for (i = 0; i < _PB_M-1; i++)
    {
      corr[i][i] = SCALAR_VAL(1.0);
      for (j = i+1; j < _PB_M; j++)
        {
          corr[i][j] = SCALAR_VAL(0.0);
          for (k = 0; k < _PB_N; k++)
            corr[i][j] += (data[k][i] * data[k][j]);
          corr[j][i] = corr[i][j];
        }
    }
  corr[_PB_M-1][_PB_M-1] = SCALAR_VAL(1.0);

}


int main(int argc, char** argv)
{
  /* Retrieve problem size. */
  int n = N;
  int m = M;

  /* Variable declaration/allocation. */
  DATA_TYPE float_n;
  POLYBENCH_2D_ARRAY_DECL(data,DATA_TYPE,N,M,n,m);
  POLYBENCH_2D_ARRAY_DECL(corr,DATA_TYPE,M,M,m,m);
  POLYBENCH_1D_ARRAY_DECL(mean,DATA_TYPE,M,m);
  POLYBENCH_1D_ARRAY_DECL(stddev,DATA_TYPE,M,m);

  /* Initialize array(s). */
  init_array (m, n, &float_n, POLYBENCH_ARRAY(data));

  /* Start timer. */
  polybench_start_instruments;

  /* Run kernel. */
  kernel_correlation (m, n, float_n,
		      POLYBENCH_ARRAY(data),
		      POLYBENCH_ARRAY(corr),
		      POLYBENCH_ARRAY(mean),
		      POLYBENCH_ARRAY(stddev));

  /* Stop and print timer. */
  polybench_stop_instruments;
  polybench_print_instruments;

  /* Prevent dead-code elimination. All live-out data must be printed
     by the function call in argument. */
  polybench_prevent_dce(print_array(m, POLYBENCH_ARRAY(corr)));

  /* Be clean. */
  POLYBENCH_FREE_ARRAY(data);
  POLYBENCH_FREE_ARRAY(corr);
  POLYBENCH_FREE_ARRAY(mean);
  POLYBENCH_FREE_ARRAY(stddev);

  return 0;
}

