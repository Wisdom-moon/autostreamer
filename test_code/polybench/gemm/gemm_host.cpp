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
  read_cl_file();
  cl_initialization();
  cl_load_prog();

  printf("%d\t%d\t%d\t", (((ni)-1)-0 + 1), 1, tasks);
  cl_mem A_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, (((ni)-1)+ 1)* sizeof (double [1200]), NULL, NULL);
  cl_mem B_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, (((nk)-1)+ 1)* sizeof (double [1100]), NULL, NULL);
  cl_mem C_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, (((ni)-1)+ 1)* sizeof (double [1100]), NULL, NULL);
  int i, j, k;

//BLAS PARAMS
//TRANSA = 'N'
//TRANSB = 'N'
// => Form C := alpha*A*B + beta*C,
//A is NIxNK
//B is NKxNJ
//C is NIxNJ
errcode = clEnqueueWriteBuffer(clCommandQue[0], B_mem_obj, CL_TRUE, 0,
(((nk)-1)+ 1)* sizeof (double [1100]), 
B, 0, NULL, NULL);
size_t localThreads[1] = {8};
clSetKernelArg(clKernel, 0, sizeof(int), &ni);
clSetKernelArg(clKernel, 1, sizeof(int), &nj);
clSetKernelArg(clKernel, 2, sizeof(double), &beta);
clSetKernelArg(clKernel, 3, sizeof(int), &nk);
clSetKernelArg(clKernel, 4, sizeof(double), &alpha);
clSetKernelArg(clKernel, 5, sizeof(cl_mem), (void *) &C_mem_obj);
clSetKernelArg(clKernel, 6, sizeof(cl_mem), (void *) &A_mem_obj);
clSetKernelArg(clKernel, 7, sizeof(cl_mem), (void *) &B_mem_obj);
DeltaT();
for (int i = 0; i < tasks; i++)
{
  size_t globalOffset[1] = {i*(((ni)-1)-0 + 1)/tasks+0};
  size_t globalThreads[1] = {(((ni)-1)-0 + 1)/tasks};
  clEnqueueWriteBuffer(clCommandQue[i], A_mem_obj, CL_FALSE, i*(((ni)-1)+ 1)* sizeof (double [1200])/tasks, (((ni)-1)+ 1)* sizeof (double [1200])/tasks, &A[i*(((ni)-1)-0 + 1)/tasks][0], 0, NULL, NULL);
  clEnqueueWriteBuffer(clCommandQue[i], C_mem_obj, CL_FALSE, i*(((ni)-1)+ 1)* sizeof (double [1100])/tasks, (((ni)-1)+ 1)* sizeof (double [1100])/tasks, &C[i*(((ni)-1)-0 + 1)/tasks][0], 0, NULL, NULL);
  clEnqueueNDRangeKernel(clCommandQue[i], clKernel, 1, globalOffset, globalThreads, localThreads, 0, NULL, NULL);
  clEnqueueReadBuffer(clCommandQue[i], C_mem_obj, CL_FALSE, i*(((ni)-1)+ 1)* sizeof (double [1100])/tasks, (((ni)-1)+ 1)* sizeof (double [1100])/tasks, &C[i*(((ni)-1)-0 + 1)/tasks][0], 0, NULL, NULL);
}
for (int i = 0; i < tasks; i++)
  clFinish(clCommandQue[i]);
printf("%f\n", DeltaT());

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

