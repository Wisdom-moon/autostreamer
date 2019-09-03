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
/* trisolv.c: this file is part of PolyBench/C */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

/* Include polybench common header. */
#include <polybench.h>

/* Include benchmark-specific header. */
#include "trisolv.h"


/* Array initialization. */
static
void init_array(int n,
		DATA_TYPE POLYBENCH_2D(L,N,N,n,n),
		DATA_TYPE POLYBENCH_1D(x,N,n),
		DATA_TYPE POLYBENCH_1D(b,N,n))
{
  int i, j;

  for (i = 0; i < n; i++)
    {
      x[i] = - 999;
      b[i] =  i ;
      for (j = 0; j <= i; j++)
	L[i][j] = (DATA_TYPE) (i+n-j+1)*2/n;
    }
}


/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static
void print_array(int n,
		 DATA_TYPE POLYBENCH_1D(x,N,n))

{
  int i;

  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("x");
  for (i = 0; i < n; i++) {
    fprintf (POLYBENCH_DUMP_TARGET, DATA_PRINTF_MODIFIER, x[i]);
    if (i % 20 == 0) fprintf (POLYBENCH_DUMP_TARGET, "\n");
  }
  POLYBENCH_DUMP_END("x");
  POLYBENCH_DUMP_FINISH;
}


/* Main computational kernel. The whole function will be timed,
   including the call and return. */
static
void kernel_trisolv(int n,
		    DATA_TYPE POLYBENCH_2D(L,N,N,n,n),
		    DATA_TYPE POLYBENCH_1D(x,N,n),
		    DATA_TYPE POLYBENCH_1D(b,N,n))
{
  read_cl_file();
  cl_initialization();
  cl_load_prog();

  printf("%d\t%d\t%d\t", (((n)-1)-0 + 1), 1, tasks);
  cl_mem L_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, (((n)-1)+ 1)* sizeof (double [2000]), NULL, NULL);
  cl_mem b_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, (((n)-1)+ 1)* sizeof (double ), NULL, NULL);
  cl_mem x_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, (((n)-1)+ 1)* sizeof (double ), NULL, NULL);
  int i, j;

errcode = clEnqueueWriteBuffer(clCommandQue[0], x_mem_obj, CL_TRUE, 0,
(((n)-1)+ 1)* sizeof (double ), 
x, 0, NULL, NULL);
size_t localThreads[1] = {8};
clSetKernelArg(clKernel, 0, sizeof(int), &n);
clSetKernelArg(clKernel, 1, sizeof(cl_mem), (void *) &x_mem_obj);
clSetKernelArg(clKernel, 2, sizeof(cl_mem), (void *) &b_mem_obj);
clSetKernelArg(clKernel, 3, sizeof(cl_mem), (void *) &L_mem_obj);
DeltaT();
for (int i = 0; i < tasks; i++)
{
  size_t globalOffset[1] = {i*(((n)-1)-0 + 1)/tasks+0};
  size_t globalThreads[1] = {(((n)-1)-0 + 1)/tasks};
  clEnqueueWriteBuffer(clCommandQue[i], L_mem_obj, CL_FALSE, i*(((n)-1)+ 1)* sizeof (double [2000])/tasks, (((n)-1)+ 1)* sizeof (double [2000])/tasks, &L[i*(((n)-1)-0 + 1)/tasks][0], 0, NULL, NULL);
  clEnqueueWriteBuffer(clCommandQue[i], b_mem_obj, CL_FALSE, i*(((n)-1)+ 1)* sizeof (double )/tasks, (((n)-1)+ 1)* sizeof (double )/tasks, &b[i*(((n)-1)-0 + 1)/tasks], 0, NULL, NULL);
  clEnqueueNDRangeKernel(clCommandQue[i], clKernel, 1, globalOffset, globalThreads, localThreads, 0, NULL, NULL);
}
for (int i = 0; i < tasks; i++)
  clFinish(clCommandQue[i]);
printf("%f\n", DeltaT());
errcode = clEnqueueReadBuffer(clCommandQue[0], x_mem_obj, CL_TRUE, 0,
(((n)-1)+ 1)* sizeof (double ), 
x, 0, NULL, NULL);
clFinish(clCommandQue[0]);

}


int main(int argc, char** argv)
{
  /* Retrieve problem size. */
  int n = N;

  /* Variable declaration/allocation. */
  POLYBENCH_2D_ARRAY_DECL(L, DATA_TYPE, N, N, n, n);
  POLYBENCH_1D_ARRAY_DECL(x, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL(b, DATA_TYPE, N, n);


  /* Initialize array(s). */
  init_array (n, POLYBENCH_ARRAY(L), POLYBENCH_ARRAY(x), POLYBENCH_ARRAY(b));

  /* Start timer. */
  polybench_start_instruments;

  /* Run kernel. */
  kernel_trisolv (n, POLYBENCH_ARRAY(L), POLYBENCH_ARRAY(x), POLYBENCH_ARRAY(b));

  /* Stop and print timer. */
  polybench_stop_instruments;
  polybench_print_instruments;

  /* Prevent dead-code elimination. All live-out data must be printed
     by the function call in argument. */
  polybench_prevent_dce(print_array(n, POLYBENCH_ARRAY(x)));

  /* Be clean. */
  POLYBENCH_FREE_ARRAY(L);
  POLYBENCH_FREE_ARRAY(x);
  POLYBENCH_FREE_ARRAY(b);

  return 0;
}

