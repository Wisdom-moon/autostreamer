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
  read_cl_file();
  cl_initialization();
  cl_load_prog();

  printf("%d\t%d\t%d\t", (((m)-1)-0 + 1), 1, tasks);
  cl_mem A_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, (((m)-1)+ 1)* sizeof (double [2100]), NULL, NULL);
  cl_mem tmp_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, (((m)-1)+ 1)* sizeof (double ), NULL, NULL);
  cl_mem x_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, (((n)-1)+ 1)* sizeof (double ), NULL, NULL);
  cl_mem y_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, (((n)-1)+ 1)* sizeof (double ), NULL, NULL);
  int i, j;

  for (i = 0; i < _PB_N; i++)
    y[i] = 0;
errcode = clEnqueueWriteBuffer(clCommandQue[0], x_mem_obj, CL_TRUE, 0,
(((n)-1)+ 1)* sizeof (double ), 
x, 0, NULL, NULL);
errcode = clEnqueueWriteBuffer(clCommandQue[0], y_mem_obj, CL_TRUE, 0,
(((n)-1)+ 1)* sizeof (double ), 
y, 0, NULL, NULL);
size_t localThreads[1] = {8};
clSetKernelArg(clKernel, 0, sizeof(int), &m);
clSetKernelArg(clKernel, 1, sizeof(int), &n);
clSetKernelArg(clKernel, 2, sizeof(cl_mem), (void *) &tmp_mem_obj);
clSetKernelArg(clKernel, 3, sizeof(cl_mem), (void *) &A_mem_obj);
clSetKernelArg(clKernel, 4, sizeof(cl_mem), (void *) &x_mem_obj);
clSetKernelArg(clKernel, 5, sizeof(cl_mem), (void *) &y_mem_obj);
DeltaT();
for (int i = 0; i < tasks; i++)
{
  size_t globalOffset[1] = {i*(((m)-1)-0 + 1)/tasks+0};
  size_t globalThreads[1] = {(((m)-1)-0 + 1)/tasks};
  clEnqueueWriteBuffer(clCommandQue[i], A_mem_obj, CL_FALSE, i*(((m)-1)+ 1)* sizeof (double [2100])/tasks, (((m)-1)+ 1)* sizeof (double [2100])/tasks, &A[i*(((m)-1)-0 + 1)/tasks][0], 0, NULL, NULL);
  clEnqueueWriteBuffer(clCommandQue[i], tmp_mem_obj, CL_FALSE, i*(((m)-1)+ 1)* sizeof (double )/tasks, (((m)-1)+ 1)* sizeof (double )/tasks, &tmp[i*(((m)-1)-0 + 1)/tasks], 0, NULL, NULL);
  clEnqueueNDRangeKernel(clCommandQue[i], clKernel, 1, globalOffset, globalThreads, localThreads, 0, NULL, NULL);
  clEnqueueReadBuffer(clCommandQue[i], tmp_mem_obj, CL_FALSE, i*(((m)-1)+ 1)* sizeof (double )/tasks, (((m)-1)+ 1)* sizeof (double )/tasks, &tmp[i*(((m)-1)-0 + 1)/tasks], 0, NULL, NULL);
}
for (int i = 0; i < tasks; i++)
  clFinish(clCommandQue[i]);
printf("%f\n", DeltaT());
errcode = clEnqueueReadBuffer(clCommandQue[0], y_mem_obj, CL_TRUE, 0,
(((n)-1)+ 1)* sizeof (double ), 
y, 0, NULL, NULL);
clFinish(clCommandQue[0]);

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

