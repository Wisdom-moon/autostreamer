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
  read_cl_file();
  cl_initialization();
  cl_load_prog();

  printf("%d\t%d\t%d\t", (((n)-1)-0 + 1), 1, tasks);
  cl_mem A_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, (((n)-1)+ 1)* sizeof (double [2000]), NULL, NULL);
  cl_mem x1_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, (((n)-1)+ 1)* sizeof (double ), NULL, NULL);
  cl_mem x2_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, (((n)-1)+ 1)* sizeof (double ), NULL, NULL);
  cl_mem y_1_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, (((n)-1)+ 1)* sizeof (double ), NULL, NULL);
  cl_mem y_2_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, (((n)-1)+ 1)* sizeof (double ), NULL, NULL);
  int i, j;

errcode = clEnqueueWriteBuffer(clCommandQue[0], A_mem_obj, CL_TRUE, 0,
(((n)-1)+ 1)* sizeof (double [2000]), 
A, 0, NULL, NULL);
errcode = clEnqueueWriteBuffer(clCommandQue[0], y_1_mem_obj, CL_TRUE, 0,
(((n)-1)+ 1)* sizeof (double ), 
y_1, 0, NULL, NULL);
errcode = clEnqueueWriteBuffer(clCommandQue[0], y_2_mem_obj, CL_TRUE, 0,
(((n)-1)+ 1)* sizeof (double ), 
y_2, 0, NULL, NULL);
size_t localThreads[1] = {8};
clSetKernelArg(clKernel, 0, sizeof(int), &n);
clSetKernelArg(clKernel, 1, sizeof(cl_mem), (void *) &x1_mem_obj);
clSetKernelArg(clKernel, 2, sizeof(cl_mem), (void *) &A_mem_obj);
clSetKernelArg(clKernel, 3, sizeof(cl_mem), (void *) &y_1_mem_obj);
clSetKernelArg(clKernel, 4, sizeof(cl_mem), (void *) &x2_mem_obj);
clSetKernelArg(clKernel, 5, sizeof(cl_mem), (void *) &y_2_mem_obj);
DeltaT();
for (int i = 0; i < tasks; i++)
{
  size_t globalOffset[1] = {i*(((n)-1)-0 + 1)/tasks+0};
  size_t globalThreads[1] = {(((n)-1)-0 + 1)/tasks};
  clEnqueueWriteBuffer(clCommandQue[i], x1_mem_obj, CL_FALSE, i*(((n)-1)+ 1)* sizeof (double )/tasks, (((n)-1)+ 1)* sizeof (double )/tasks, &x1[i*(((n)-1)-0 + 1)/tasks], 0, NULL, NULL);
  clEnqueueWriteBuffer(clCommandQue[i], x2_mem_obj, CL_FALSE, i*(((n)-1)+ 1)* sizeof (double )/tasks, (((n)-1)+ 1)* sizeof (double )/tasks, &x2[i*(((n)-1)-0 + 1)/tasks], 0, NULL, NULL);
  clEnqueueNDRangeKernel(clCommandQue[i], clKernel, 1, globalOffset, globalThreads, localThreads, 0, NULL, NULL);
  clEnqueueReadBuffer(clCommandQue[i], x1_mem_obj, CL_FALSE, i*(((n)-1)+ 1)* sizeof (double )/tasks, (((n)-1)+ 1)* sizeof (double )/tasks, &x1[i*(((n)-1)-0 + 1)/tasks], 0, NULL, NULL);
  clEnqueueReadBuffer(clCommandQue[i], x2_mem_obj, CL_FALSE, i*(((n)-1)+ 1)* sizeof (double )/tasks, (((n)-1)+ 1)* sizeof (double )/tasks, &x2[i*(((n)-1)-0 + 1)/tasks], 0, NULL, NULL);
}
for (int i = 0; i < tasks; i++)
  clFinish(clCommandQue[i]);
printf("%f\n", DeltaT());

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

