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
  read_cl_file();
  cl_initialization();
  cl_load_prog();

  printf("%d\t%d\t%d\t", (((n)-1)-0 + 1), 1, tasks);
  cl_mem A_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, (((n)-1)+ 1)* sizeof (double [1300]), NULL, NULL);
  cl_mem B_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, (((n)-1)+ 1)* sizeof (double [1300]), NULL, NULL);
  cl_mem tmp_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, (((n)-1)+ 1)* sizeof (double ), NULL, NULL);
  cl_mem x_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, (((n)-1)+ 1)* sizeof (double ), NULL, NULL);
  cl_mem y_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, (((n)-1)+ 1)* sizeof (double ), NULL, NULL);
  int i, j;

errcode = clEnqueueWriteBuffer(clCommandQue[0], x_mem_obj, CL_TRUE, 0,
(((n)-1)+ 1)* sizeof (double ), 
x, 0, NULL, NULL);
size_t localThreads[1] = {8};
clSetKernelArg(clKernel, 0, sizeof(int), &n);
clSetKernelArg(clKernel, 1, sizeof(double), &alpha);
clSetKernelArg(clKernel, 2, sizeof(double), &beta);
clSetKernelArg(clKernel, 3, sizeof(cl_mem), (void *) &tmp_mem_obj);
clSetKernelArg(clKernel, 4, sizeof(cl_mem), (void *) &y_mem_obj);
clSetKernelArg(clKernel, 5, sizeof(cl_mem), (void *) &A_mem_obj);
clSetKernelArg(clKernel, 6, sizeof(cl_mem), (void *) &x_mem_obj);
clSetKernelArg(clKernel, 7, sizeof(cl_mem), (void *) &B_mem_obj);
DeltaT();
for (int i = 0; i < tasks; i++)
{
  size_t globalOffset[1] = {i*(((n)-1)-0 + 1)/tasks+0};
  size_t globalThreads[1] = {(((n)-1)-0 + 1)/tasks};
  clEnqueueWriteBuffer(clCommandQue[i], A_mem_obj, CL_FALSE, i*(((n)-1)+ 1)* sizeof (double [1300])/tasks, (((n)-1)+ 1)* sizeof (double [1300])/tasks, &A[i*(((n)-1)-0 + 1)/tasks][0], 0, NULL, NULL);
  clEnqueueWriteBuffer(clCommandQue[i], B_mem_obj, CL_FALSE, i*(((n)-1)+ 1)* sizeof (double [1300])/tasks, (((n)-1)+ 1)* sizeof (double [1300])/tasks, &B[i*(((n)-1)-0 + 1)/tasks][0], 0, NULL, NULL);
  clEnqueueWriteBuffer(clCommandQue[i], tmp_mem_obj, CL_FALSE, i*(((n)-1)+ 1)* sizeof (double )/tasks, (((n)-1)+ 1)* sizeof (double )/tasks, &tmp[i*(((n)-1)-0 + 1)/tasks], 0, NULL, NULL);
  clEnqueueWriteBuffer(clCommandQue[i], y_mem_obj, CL_FALSE, i*(((n)-1)+ 1)* sizeof (double )/tasks, (((n)-1)+ 1)* sizeof (double )/tasks, &y[i*(((n)-1)-0 + 1)/tasks], 0, NULL, NULL);
  clEnqueueNDRangeKernel(clCommandQue[i], clKernel, 1, globalOffset, globalThreads, localThreads, 0, NULL, NULL);
  clEnqueueReadBuffer(clCommandQue[i], tmp_mem_obj, CL_FALSE, i*(((n)-1)+ 1)* sizeof (double )/tasks, (((n)-1)+ 1)* sizeof (double )/tasks, &tmp[i*(((n)-1)-0 + 1)/tasks], 0, NULL, NULL);
  clEnqueueReadBuffer(clCommandQue[i], y_mem_obj, CL_FALSE, i*(((n)-1)+ 1)* sizeof (double )/tasks, (((n)-1)+ 1)* sizeof (double )/tasks, &y[i*(((n)-1)-0 + 1)/tasks], 0, NULL, NULL);
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

