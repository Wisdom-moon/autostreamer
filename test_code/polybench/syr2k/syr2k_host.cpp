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
/* syr2k.c: this file is part of PolyBench/C */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

/* Include polybench common header. */
#include <polybench.h>

/* Include benchmark-specific header. */
#include "syr2k.h"


/* Array initialization. */
static
void init_array(int n, int m,
		DATA_TYPE *alpha,
		DATA_TYPE *beta,
		DATA_TYPE POLYBENCH_2D(C,N,N,n,n),
		DATA_TYPE POLYBENCH_2D(A,N,M,n,m),
		DATA_TYPE POLYBENCH_2D(B,N,M,n,m))
{
  int i, j;

  *alpha = 1.5;
  *beta = 1.2;
  for (i = 0; i < n; i++)
    for (j = 0; j < m; j++) {
      A[i][j] = (DATA_TYPE) ((i*j+1)%n) / n;
      B[i][j] = (DATA_TYPE) ((i*j+2)%m) / m;
    }
  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++) {
      C[i][j] = (DATA_TYPE) ((i*j+3)%n) / m;
    }
}


/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static
void print_array(int n,
		 DATA_TYPE POLYBENCH_2D(C,N,N,n,n))
{
  int i, j;

  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("C");
  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++) {
	if ((i * n + j) % 20 == 0) fprintf (POLYBENCH_DUMP_TARGET, "\n");
	fprintf (POLYBENCH_DUMP_TARGET, DATA_PRINTF_MODIFIER, C[i][j]);
    }
  POLYBENCH_DUMP_END("C");
  POLYBENCH_DUMP_FINISH;
}


/* Main computational kernel. The whole function will be timed,
   including the call and return. */
static
void kernel_syr2k(int n, int m,
		  DATA_TYPE alpha,
		  DATA_TYPE beta,
		  DATA_TYPE POLYBENCH_2D(C,N,N,n,n),
		  DATA_TYPE POLYBENCH_2D(A,N,M,n,m),
		  DATA_TYPE POLYBENCH_2D(B,N,M,n,m))
{
  read_cl_file();
  cl_initialization();
  cl_load_prog();

  printf("%d\t%d\t%d\t", (((n)-1)-0 + 1), 1, tasks);
  cl_mem A_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, ((((n)-1))+ 1)* sizeof (double [1000]), NULL, NULL);
  cl_mem B_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, (((n)-1)+ 1)* sizeof (double [1000]), NULL, NULL);
  cl_mem C_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, (((n)-1)+ 1)* sizeof (double [1200]), NULL, NULL);
  int i, j, k;

//BLAS PARAMS
//UPLO  = 'L'
//TRANS = 'N'
//A is NxM
//B is NxM
//C is NxN
errcode = clEnqueueWriteBuffer(clCommandQue[0], A_mem_obj, CL_TRUE, 0,
((((n)-1))+ 1)* sizeof (double [1000]), 
A, 0, NULL, NULL);
errcode = clEnqueueWriteBuffer(clCommandQue[0], B_mem_obj, CL_TRUE, 0,
(((n)-1)+ 1)* sizeof (double [1000]), 
B, 0, NULL, NULL);
size_t localThreads[1] = {8};
clSetKernelArg(clKernel, 0, sizeof(int), &n);
clSetKernelArg(clKernel, 1, sizeof(double), &beta);
clSetKernelArg(clKernel, 2, sizeof(int), &m);
clSetKernelArg(clKernel, 3, sizeof(double), &alpha);
clSetKernelArg(clKernel, 4, sizeof(cl_mem), (void *) &C_mem_obj);
clSetKernelArg(clKernel, 5, sizeof(cl_mem), (void *) &A_mem_obj);
clSetKernelArg(clKernel, 6, sizeof(cl_mem), (void *) &B_mem_obj);
DeltaT();
for (int i = 0; i < tasks; i++)
{
  size_t globalOffset[1] = {i*(((n)-1)-0 + 1)/tasks+0};
  size_t globalThreads[1] = {(((n)-1)-0 + 1)/tasks};
  clEnqueueWriteBuffer(clCommandQue[i], C_mem_obj, CL_FALSE, i*(((n)-1)+ 1)* sizeof (double [1200])/tasks, (((n)-1)+ 1)* sizeof (double [1200])/tasks, &C[i*(((n)-1)-0 + 1)/tasks][0], 0, NULL, NULL);
  clEnqueueNDRangeKernel(clCommandQue[i], clKernel, 1, globalOffset, globalThreads, localThreads, 0, NULL, NULL);
  clEnqueueReadBuffer(clCommandQue[i], C_mem_obj, CL_FALSE, i*(((n)-1)+ 1)* sizeof (double [1200])/tasks, (((n)-1)+ 1)* sizeof (double [1200])/tasks, &C[i*(((n)-1)-0 + 1)/tasks][0], 0, NULL, NULL);
}
for (int i = 0; i < tasks; i++)
  clFinish(clCommandQue[i]);
printf("%f\n", DeltaT());

}


int main(int argc, char** argv)
{
  /* Retrieve problem size. */
  int n = N;
  int m = M;

  /* Variable declaration/allocation. */
  DATA_TYPE alpha;
  DATA_TYPE beta;
  POLYBENCH_2D_ARRAY_DECL(C,DATA_TYPE,N,N,n,n);
  POLYBENCH_2D_ARRAY_DECL(A,DATA_TYPE,N,M,n,m);
  POLYBENCH_2D_ARRAY_DECL(B,DATA_TYPE,N,M,n,m);

  /* Initialize array(s). */
  init_array (n, m, &alpha, &beta,
	      POLYBENCH_ARRAY(C),
	      POLYBENCH_ARRAY(A),
	      POLYBENCH_ARRAY(B));

  /* Start timer. */
  polybench_start_instruments;

  /* Run kernel. */
  kernel_syr2k (n, m,
		alpha, beta,
		POLYBENCH_ARRAY(C),
		POLYBENCH_ARRAY(A),
		POLYBENCH_ARRAY(B));

  /* Stop and print timer. */
  polybench_stop_instruments;
  polybench_print_instruments;

  /* Prevent dead-code elimination. All live-out data must be printed
     by the function call in argument. */
  polybench_prevent_dce(print_array(n, POLYBENCH_ARRAY(C)));

  /* Be clean. */
  POLYBENCH_FREE_ARRAY(C);
  POLYBENCH_FREE_ARRAY(A);
  POLYBENCH_FREE_ARRAY(B);

  return 0;
}

