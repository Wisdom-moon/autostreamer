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
/* adi.c: this file is part of PolyBench/C */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

/* Include polybench common header. */
#include <polybench.h>

/* Include benchmark-specific header. */
#include "adi.h"


/* Array initialization. */
static
void init_array (int n,
		 DATA_TYPE POLYBENCH_2D(u,N,N,n,n))
{
  int i, j;

  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++)
      {
	u[i][j] =  (DATA_TYPE)(i + n-j) / n;
      }
}


/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static
void print_array(int n,
		 DATA_TYPE POLYBENCH_2D(u,N,N,n,n))

{
  int i, j;

  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("u");
  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++) {
      if ((i * n + j) % 20 == 0) fprintf(POLYBENCH_DUMP_TARGET, "\n");
      fprintf (POLYBENCH_DUMP_TARGET, DATA_PRINTF_MODIFIER, u[i][j]);
    }
  POLYBENCH_DUMP_END("u");
  POLYBENCH_DUMP_FINISH;
}


/* Main computational kernel. The whole function will be timed,
   including the call and return. */
/* Based on a Fortran code fragment from Figure 5 of
 * "Automatic Data and Computation Decomposition on Distributed Memory Parallel Computers"
 * by Peizong Lee and Zvi Meir Kedem, TOPLAS, 2002
 */
static
void kernel_adi(int tsteps, int n,
		DATA_TYPE POLYBENCH_2D(u,N,N,n,n),
		DATA_TYPE POLYBENCH_2D(v,N,N,n,n),
		DATA_TYPE POLYBENCH_2D(p,N,N,n,n),
		DATA_TYPE POLYBENCH_2D(q,N,N,n,n))
{
  read_cl_file();
  cl_initialization();
  cl_load_prog();

  printf("%d\t%d\t%d\t", (((n-1)-1)-1 + 1), 1, tasks);
  cl_mem p_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, (((n-1)-1)+ 1)* sizeof (double [1000]), NULL, NULL);
  cl_mem q_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, (((n-1)-1)+ 1)* sizeof (double [1000]), NULL, NULL);
  cl_mem u_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, (((n-1)-1)+ 1)* sizeof (double [1000]), NULL, NULL);
  cl_mem v_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, (((n-1)-1)+1+ 1)* sizeof (double [1000]), NULL, NULL);
  int t, i, j;
  DATA_TYPE DX, DY, DT;
  DATA_TYPE B1, B2;
  DATA_TYPE mul1, mul2;
  DATA_TYPE a, b, c, d, e, f;


  DX = SCALAR_VAL(1.0)/(DATA_TYPE)_PB_N;
  DY = SCALAR_VAL(1.0)/(DATA_TYPE)_PB_N;
  DT = SCALAR_VAL(1.0)/(DATA_TYPE)_PB_TSTEPS;
  B1 = SCALAR_VAL(2.0);
  B2 = SCALAR_VAL(1.0);
  mul1 = B1 * DT / (DX * DX);
  mul2 = B2 * DT / (DY * DY);

  a = -mul1 /  SCALAR_VAL(2.0);
  b = SCALAR_VAL(1.0)+mul1;
  c = a;
  d = -mul2 / SCALAR_VAL(2.0);
  e = SCALAR_VAL(1.0)+mul2;
  f = d;

 for (t=1; t<=_PB_TSTEPS; t++) {
    //Column Sweep
    for (i=1; i<_PB_N-1; i++) {
      v[0][i] = SCALAR_VAL(1.0);
      p[i][0] = SCALAR_VAL(0.0);
      q[i][0] = v[0][i];
      for (j=1; j<_PB_N-1; j++) {
        p[i][j] = -c / (a*p[i][j-1]+b);
        q[i][j] = (-d*u[j][i-1]+(SCALAR_VAL(1.0)+SCALAR_VAL(2.0)*d)*u[j][i] - f*u[j][i+1]-a*q[i][j-1])/(a*p[i][j-1]+b);
      }

      v[_PB_N-1][i] = SCALAR_VAL(1.0);
      for (j=_PB_N-2; j>=1; j--) {
        v[j][i] = p[i][j] * v[j+1][i] + q[i][j];
      }
    }
    //Row Sweep
errcode = clEnqueueWriteBuffer(clCommandQue[0], v_mem_obj, CL_TRUE, 0,
(((n-1)-1)+1+ 1)* sizeof (double [1000]), 
v, 0, NULL, NULL);
size_t localThreads[1] = {8};
clSetKernelArg(clKernel, 0, sizeof(int), &n);
clSetKernelArg(clKernel, 1, sizeof(double), &f);
clSetKernelArg(clKernel, 2, sizeof(double), &d);
clSetKernelArg(clKernel, 3, sizeof(double), &e);
clSetKernelArg(clKernel, 4, sizeof(double), &a);
clSetKernelArg(clKernel, 5, sizeof(double), &c);
clSetKernelArg(clKernel, 6, sizeof(cl_mem), (void *) &u_mem_obj);
clSetKernelArg(clKernel, 7, sizeof(cl_mem), (void *) &p_mem_obj);
clSetKernelArg(clKernel, 8, sizeof(cl_mem), (void *) &q_mem_obj);
clSetKernelArg(clKernel, 9, sizeof(cl_mem), (void *) &v_mem_obj);
DeltaT();
for (int i = 0; i < tasks; i++)
{
  size_t globalOffset[1] = {i*(((n-1)-1)-1 + 1)/tasks+1};
  size_t globalThreads[1] = {(((n-1)-1)-1 + 1)/tasks};
  clEnqueueWriteBuffer(clCommandQue[i], p_mem_obj, CL_FALSE, i*(((n-1)-1)+ 1)* sizeof (double [1000])/tasks, (((n-1)-1)+ 1)* sizeof (double [1000])/tasks, &p[i*(((n-1)-1)-1 + 1)/tasks][0], 0, NULL, NULL);
  clEnqueueWriteBuffer(clCommandQue[i], q_mem_obj, CL_FALSE, i*(((n-1)-1)+ 1)* sizeof (double [1000])/tasks, (((n-1)-1)+ 1)* sizeof (double [1000])/tasks, &q[i*(((n-1)-1)-1 + 1)/tasks][0], 0, NULL, NULL);
  clEnqueueWriteBuffer(clCommandQue[i], u_mem_obj, CL_FALSE, i*(((n-1)-1)+ 1)* sizeof (double [1000])/tasks, (((n-1)-1)+ 1)* sizeof (double [1000])/tasks, &u[i*(((n-1)-1)-1 + 1)/tasks][0], 0, NULL, NULL);
  clEnqueueNDRangeKernel(clCommandQue[i], clKernel, 1, globalOffset, globalThreads, localThreads, 0, NULL, NULL);
  clEnqueueReadBuffer(clCommandQue[i], p_mem_obj, CL_FALSE, i*(((n-1)-1)+ 1)* sizeof (double [1000])/tasks, (((n-1)-1)+ 1)* sizeof (double [1000])/tasks, &p[i*(((n-1)-1)-1 + 1)/tasks][0], 0, NULL, NULL);
  clEnqueueReadBuffer(clCommandQue[i], q_mem_obj, CL_FALSE, i*(((n-1)-1)+ 1)* sizeof (double [1000])/tasks, (((n-1)-1)+ 1)* sizeof (double [1000])/tasks, &q[i*(((n-1)-1)-1 + 1)/tasks][0], 0, NULL, NULL);
  clEnqueueReadBuffer(clCommandQue[i], u_mem_obj, CL_FALSE, i*(((n-1)-1)+ 1)* sizeof (double [1000])/tasks, (((n-1)-1)+ 1)* sizeof (double [1000])/tasks, &u[i*(((n-1)-1)-1 + 1)/tasks][0], 0, NULL, NULL);
}
for (int i = 0; i < tasks; i++)
  clFinish(clCommandQue[i]);
printf("%f\n", DeltaT());
  }
}


int main(int argc, char** argv)
{
  /* Retrieve problem size. */
  int n = N;
  int tsteps = TSTEPS;

  /* Variable declaration/allocation. */
  POLYBENCH_2D_ARRAY_DECL(u, DATA_TYPE, N, N, n, n);
  POLYBENCH_2D_ARRAY_DECL(v, DATA_TYPE, N, N, n, n);
  POLYBENCH_2D_ARRAY_DECL(p, DATA_TYPE, N, N, n, n);
  POLYBENCH_2D_ARRAY_DECL(q, DATA_TYPE, N, N, n, n);


  /* Initialize array(s). */
  init_array (n, POLYBENCH_ARRAY(u));

  /* Start timer. */
  polybench_start_instruments;

  /* Run kernel. */
  kernel_adi (tsteps, n, POLYBENCH_ARRAY(u), POLYBENCH_ARRAY(v), POLYBENCH_ARRAY(p), POLYBENCH_ARRAY(q));

  /* Stop and print timer. */
  polybench_stop_instruments;
  polybench_print_instruments;

  /* Prevent dead-code elimination. All live-out data must be printed
     by the function call in argument. */
  polybench_prevent_dce(print_array(n, POLYBENCH_ARRAY(u)));

  /* Be clean. */
  POLYBENCH_FREE_ARRAY(u);
  POLYBENCH_FREE_ARRAY(v);
  POLYBENCH_FREE_ARRAY(p);
  POLYBENCH_FREE_ARRAY(q);

  return 0;
}

