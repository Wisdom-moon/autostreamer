#include <omp.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <cuda.h>
#include <cuda_runtime_api.h>


__global__ void my_kernel
 ( int inputLength, float * hostOutput, float * hostInput1, float * hostInput2, int length)
{

int i = blockDim.x * blockIdx.x + threadIdx.x;
if (i >= length)
  return;

  {
    hostOutput[i] = hostInput1[i] + hostInput2[i];
  }
}

int main(int argc, char *argv[]) {
  int nstreams = 2;
  cudaSetDevice(0);
  cudaSetDeviceFlags(cudaDeviceBlockingSync);
  cudaStream_t *streams = (cudaStream_t*) malloc(nstreams*sizeof(cudaStream_t));
  for (int i = 0; i < nstreams; i++) {
    cudaStreamCreate(&(streams[i]));
  }

  cudaEvent_t start_event, stop_event;
  int eventflags = cudaEventBlockingSync;
  cudaEventCreateWithFlags(&start_event, eventflags);
  cudaEventCreateWithFlags(&stop_event, eventflags);
  int inputLength = 32;
  int inputLengthBytes;
  inputLengthBytes = inputLength * sizeof(float);
  float *hostInput1;
  float *hostInput2;
  float * d_hostInput1;
  cudaMalloc((void **)&d_hostInput1, (inputLengthBytes));
  float * d_hostInput2;
  cudaMalloc((void **)&d_hostInput2, (inputLengthBytes));
  float * d_hostOutput;
  cudaMalloc((void **)&d_hostOutput, (inputLengthBytes));
  float *hostOutput;

  if (argc > 1) {
    inputLength = atoi (argv[1]);
  }

  hostInput1 = (float *)malloc(inputLengthBytes);
  hostInput2 = (float *)malloc(inputLengthBytes);
  hostOutput       = (float *)malloc(inputLengthBytes);

  for(int i=0; i<inputLength; i++)
  {
    hostInput1[i] = (float)(i % 11);
    hostInput2[i] = (float)(i % 31);
  }


int threadsPerBlock = 8;
for (int i = 0; i < nstreams; i++)
{
int blocksPerGrid = ((((inputLength)-1)-0 + 1)+threadsPerBlock - 1)/(nstreams*threadsPerBlock);
  cudaMemcpyAsync(d_hostInput1+i*(((inputLength)-1)-0 + 1)/nstreams, hostInput1+i*(((inputLength)-1)-0 + 1)/nstreams, (inputLengthBytes)/nstreams, cudaMemcpyHostToDevice, streams[i]);
  cudaMemcpyAsync(d_hostInput2+i*(((inputLength)-1)-0 + 1)/nstreams, hostInput2+i*(((inputLength)-1)-0 + 1)/nstreams, (inputLengthBytes)/nstreams, cudaMemcpyHostToDevice, streams[i]);
  my_kernel<<<blocksPerGrid, threadsPerBlock,0, streams[i]>>>(inputLength, d_hostOutput+i*(((inputLength)-1)-0 + 1)/nstreams, d_hostInput1+i*(((inputLength)-1)-0 + 1)/nstreams, d_hostInput2+i*(((inputLength)-1)-0 + 1)/nstreams, (((inputLength)-1)-0 + 1)/nstreams);
  cudaMemcpyAsync(hostOutput+i*(((inputLength)-1)-0 + 1)/nstreams, d_hostOutput+i*(((inputLength)-1)-0 + 1)/nstreams, (inputLengthBytes)/nstreams, cudaMemcpyDeviceToHost, streams[i]);
}
cudaEventRecord(stop_event, 0);
cudaEventSynchronize(stop_event);

  for (int i = 0; i < nstreams; i++) {
    cudaStreamDestroy(streams[i]);
  }
  cudaEventDestroy(start_event);
  cudaEventDestroy(stop_event);
  cudaFree(d_hostInput1);
  cudaFree(d_hostInput2);
  cudaFree(d_hostOutput);
  cudaDeviceReset();
  for(int i=0; i<inputLength; i++)
  {
    printf("%f  ", hostOutput[i]);
  }
  printf("\n");

  // release host memory
  if(hostInput1 != NULL) free(hostInput1);
  if(hostInput2 != NULL) free(hostInput2);
  if(hostOutput != NULL) free(hostOutput);

  return 0;
}

