#include "set_env.h"
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
  read_cl_file();
  cl_initialization();
  cl_load_prog();

  printf("%d\t%d\t%d\t", (((m)-1)-0 + 1), 1, tasks);
  cl_mem data_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, (((n)-1)+ 1)* sizeof (double [1200]), NULL, NULL);
  cl_mem mean_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, (((m)-1)+ 1)* sizeof (double ), NULL, NULL);
  cl_mem stddev_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, (((m)-1)+ 1)* sizeof (double ), NULL, NULL);
  int i, j, k;

  DATA_TYPE eps = SCALAR_VAL(0.1);


  for (j = 0; j < _PB_M; j++)
    {
      mean[j] = SCALAR_VAL(0.0);
      for (i = 0; i < _PB_N; i++)
	mean[j] += data[i][j];
      mean[j] /= float_n;
    }


errcode = clEnqueueWriteBuffer(clCommandQue[0], data_mem_obj, CL_TRUE, 0,
(((n)-1)+ 1)* sizeof (double [1200]), 
data, 0, NULL, NULL);
size_t localThreads[1] = {8};
clSetKernelArg(clKernel, 0, sizeof(int), &m);
clSetKernelArg(clKernel, 1, sizeof(int), &n);
clSetKernelArg(clKernel, 2, sizeof(double), &float_n);
clSetKernelArg(clKernel, 3, sizeof(double), &eps);
clSetKernelArg(clKernel, 4, sizeof(cl_mem), (void *) &stddev_mem_obj);
clSetKernelArg(clKernel, 5, sizeof(cl_mem), (void *) &data_mem_obj);
clSetKernelArg(clKernel, 6, sizeof(cl_mem), (void *) &mean_mem_obj);
DeltaT();
for (int i = 0; i < tasks; i++)
{
  size_t globalOffset[1] = {i*(((m)-1)-0 + 1)/tasks+0};
  size_t globalThreads[1] = {(((m)-1)-0 + 1)/tasks};
  clEnqueueWriteBuffer(clCommandQue[i], mean_mem_obj, CL_FALSE, i*(((m)-1)+ 1)* sizeof (double )/tasks, (((m)-1)+ 1)* sizeof (double )/tasks, &mean[i*(((m)-1)-0 + 1)/tasks], 0, NULL, NULL);
  clEnqueueWriteBuffer(clCommandQue[i], stddev_mem_obj, CL_FALSE, i*(((m)-1)+ 1)* sizeof (double )/tasks, (((m)-1)+ 1)* sizeof (double )/tasks, &stddev[i*(((m)-1)-0 + 1)/tasks], 0, NULL, NULL);
  clEnqueueNDRangeKernel(clCommandQue[i], clKernel, 1, globalOffset, globalThreads, localThreads, 0, NULL, NULL);
  clEnqueueReadBuffer(clCommandQue[i], stddev_mem_obj, CL_FALSE, i*(((m)-1)+ 1)* sizeof (double )/tasks, (((m)-1)+ 1)* sizeof (double )/tasks, &stddev[i*(((m)-1)-0 + 1)/tasks], 0, NULL, NULL);
}
for (int i = 0; i < tasks; i++)
  clFinish(clCommandQue[i]);
printf("%f\n", DeltaT());

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
          clReleaseMemObject(data_mem_obj);
          clReleaseMemObject(mean_mem_obj);
          clReleaseMemObject(stddev_mem_obj);
          cl_clean_up();
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

