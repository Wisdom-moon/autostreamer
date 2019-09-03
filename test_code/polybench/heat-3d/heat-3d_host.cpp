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
/* heat-3d.c: this file is part of PolyBench/C */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

/* Include polybench common header. */
#include <polybench.h>

/* Include benchmark-specific header. */
#include "heat-3d.h"


/* Array initialization. */
static
void init_array (int n,
		 DATA_TYPE POLYBENCH_3D(A,N,N,N,n,n,n),
		 DATA_TYPE POLYBENCH_3D(B,N,N,N,n,n,n))
{
  int i, j, k;

  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++)
      for (k = 0; k < n; k++)
        A[i][j][k] = B[i][j][k] = (DATA_TYPE) (i + j + (n-k))* 10 / (n);
}


/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static
void print_array(int n,
		 DATA_TYPE POLYBENCH_3D(A,N,N,N,n,n,n))

{
  int i, j, k;

  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("A");
  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++)
      for (k = 0; k < n; k++) {
         if ((i * n * n + j * n + k) % 20 == 0) fprintf(POLYBENCH_DUMP_TARGET, "\n");
         fprintf(POLYBENCH_DUMP_TARGET, DATA_PRINTF_MODIFIER, A[i][j][k]);
      }
  POLYBENCH_DUMP_END("A");
  POLYBENCH_DUMP_FINISH;
}


/* Main computational kernel. The whole function will be timed,
   including the call and return. */
static
void kernel_heat_3d(int tsteps,
		      int n,
		      DATA_TYPE POLYBENCH_3D(A,N,N,N,n,n,n),
		      DATA_TYPE POLYBENCH_3D(B,N,N,N,n,n,n))
{
  read_cl_file();
  cl_initialization();
  cl_load_prog();

  printf("%d\t%d\t%d\t", (((n-1)-1)-1 + 1), 1, tasks);
  cl_mem A_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, (((n-1)-1)+1+ 1)* sizeof (double [120][120]), NULL, NULL);
  cl_mem B_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, (((n-1)-1)+ 1)* sizeof (double [120][120]), NULL, NULL);
  int t, i, j, k;

    for (t = 1; t <= TSTEPS; t++) {
	errcode = clEnqueueWriteBuffer(clCommandQue[0], A_mem_obj, CL_TRUE, 0,
	(((n-1)-1)+1+ 1)* sizeof (double [120][120]), 
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
	  clEnqueueReadBuffer(clCommandQue[i], B_mem_obj, CL_FALSE, i*(((n-1)-1)+ 1)* sizeof (double [120][120])/tasks, (((n-1)-1)+ 1)* sizeof (double [120][120])/tasks, &B[i*(((n-1)-1)-1 + 1)/tasks][0][0], 0, NULL, NULL);
	}
	for (int i = 0; i < tasks; i++)
	  clFinish(clCommandQue[i]);
	printf("%f\n", DeltaT());
        for (i = 1; i < _PB_N-1; i++) {
           for (j = 1; j < _PB_N-1; j++) {
               clReleaseMemObject(A_mem_obj);
               clReleaseMemObject(B_mem_obj);
               cl_clean_up();
               for (k = 1; k < _PB_N-1; k++) {
                   A[i][j][k] =   SCALAR_VAL(0.125) * (B[i+1][j][k] - SCALAR_VAL(2.0) * B[i][j][k] + B[i-1][j][k])
                                + SCALAR_VAL(0.125) * (B[i][j+1][k] - SCALAR_VAL(2.0) * B[i][j][k] + B[i][j-1][k])
                                + SCALAR_VAL(0.125) * (B[i][j][k+1] - SCALAR_VAL(2.0) * B[i][j][k] + B[i][j][k-1])
                                + B[i][j][k];
               }
           }
       }
    }

}


int main(int argc, char** argv)
{
  /* Retrieve problem size. */
  int n = N;
  int tsteps = TSTEPS;

  /* Variable declaration/allocation. */
  POLYBENCH_3D_ARRAY_DECL(A, DATA_TYPE, N, N, N, n, n, n);
  POLYBENCH_3D_ARRAY_DECL(B, DATA_TYPE, N, N, N, n, n, n);


  /* Initialize array(s). */
  init_array (n, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(B));

  /* Start timer. */
  polybench_start_instruments;

  /* Run kernel. */
  kernel_heat_3d (tsteps, n, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(B));

  /* Stop and print timer. */
  polybench_stop_instruments;
  polybench_print_instruments;

  /* Prevent dead-code elimination. All live-out data must be printed
     by the function call in argument. */
  polybench_prevent_dce(print_array(n, POLYBENCH_ARRAY(A)));

  /* Be clean. */
  POLYBENCH_FREE_ARRAY(A);

  return 0;
}

