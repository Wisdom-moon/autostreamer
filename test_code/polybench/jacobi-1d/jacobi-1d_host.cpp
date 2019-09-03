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
/* jacobi-1d.c: this file is part of PolyBench/C */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

/* Include polybench common header. */
#include <polybench.h>

/* Include benchmark-specific header. */
#include "jacobi-1d.h"


/* Array initialization. */
static
void init_array (int n,
		 DATA_TYPE POLYBENCH_1D(A,N,n),
		 DATA_TYPE POLYBENCH_1D(B,N,n))
{
  int i;

  for (i = 0; i < n; i++)
      {
	A[i] = ((DATA_TYPE) i+ 2) / n;
	B[i] = ((DATA_TYPE) i+ 3) / n;
      }
}


/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static
void print_array(int n,
		 DATA_TYPE POLYBENCH_1D(A,N,n))

{
  int i;

  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("A");
  for (i = 0; i < n; i++)
    {
      if (i % 20 == 0) fprintf(POLYBENCH_DUMP_TARGET, "\n");
      fprintf(POLYBENCH_DUMP_TARGET, DATA_PRINTF_MODIFIER, A[i]);
    }
  POLYBENCH_DUMP_END("A");
  POLYBENCH_DUMP_FINISH;
}


/* Main computational kernel. The whole function will be timed,
   including the call and return. */
static
void kernel_jacobi_1d(int tsteps,
			    int n,
			    DATA_TYPE POLYBENCH_1D(A,N,n),
			    DATA_TYPE POLYBENCH_1D(B,N,n))
{
  read_cl_file();
  cl_initialization();
  cl_load_prog();

  printf("%d\t%d\t%d\t", (((n-1)-1)-1 + 1), 1, tasks);
  cl_mem A_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, (((n-1)-1)+1+ 1)* sizeof (double ), NULL, NULL);
  cl_mem B_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, (((n-1)-1)+ 1)* sizeof (double ), NULL, NULL);
  int t, i;

  for (t = 0; t < _PB_TSTEPS; t++)
    {
errcode = clEnqueueWriteBuffer(clCommandQue[0], A_mem_obj, CL_TRUE, 0,
(((n-1)-1)+1+ 1)* sizeof (double ), 
A, 0, NULL, NULL);
size_t localThreads[1] = {8};
clSetKernelArg(clKernel, 0, sizeof(int), &n);
clSetKernelArg(clKernel, 1, sizeof(cl_mem), (void *) &B_mem_obj);
clSetKernelArg(clKernel, 2, sizeof(cl_mem), (void *) &A_mem_obj);
DeltaT();
for (int i = 0; i < tasks; i++)
{
  size_t globalOffset[1] = {i*(((n-1)-1)-1 + 1)/tasks+1};
  size_t globalThreads[1] = {(((n-1)-1)-1 + 1)/tasks};
  clEnqueueNDRangeKernel(clCommandQue[i], clKernel, 1, globalOffset, globalThreads, localThreads, 0, NULL, NULL);
  clEnqueueReadBuffer(clCommandQue[i], B_mem_obj, CL_FALSE, i*(((n-1)-1)+ 1)* sizeof (double )/tasks, (((n-1)-1)+ 1)* sizeof (double )/tasks, &B[i*(((n-1)-1)-1 + 1)/tasks], 0, NULL, NULL);
}
for (int i = 0; i < tasks; i++)
  clFinish(clCommandQue[i]);
printf("%f\n", DeltaT());
      clReleaseMemObject(A_mem_obj);
      clReleaseMemObject(B_mem_obj);
      cl_clean_up();
      for (i = 1; i < _PB_N - 1; i++)
	A[i] = 0.33333 * (B[i-1] + B[i] + B[i + 1]);
    }

}


int main(int argc, char** argv)
{
  /* Retrieve problem size. */
  int n = N;
  int tsteps = TSTEPS;

  /* Variable declaration/allocation. */
  POLYBENCH_1D_ARRAY_DECL(A, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL(B, DATA_TYPE, N, n);


  /* Initialize array(s). */
  init_array (n, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(B));

  /* Start timer. */
  polybench_start_instruments;

  /* Run kernel. */
  kernel_jacobi_1d(tsteps, n, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(B));

  /* Stop and print timer. */
  polybench_stop_instruments;
  polybench_print_instruments;

  /* Prevent dead-code elimination. All live-out data must be printed
     by the function call in argument. */
  polybench_prevent_dce(print_array(n, POLYBENCH_ARRAY(A)));

  /* Be clean. */
  POLYBENCH_FREE_ARRAY(A);
  POLYBENCH_FREE_ARRAY(B);

  return 0;
}

