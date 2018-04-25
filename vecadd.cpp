#include <omp.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  int inputLength = 32;
  int inputLengthBytes;
  float *hostInput1;
  float *hostInput2;
  float *hostOutput;

  if (argc > 1) {
    inputLength = atoi (argv[1]);
  }

  inputLengthBytes = inputLength * sizeof(float);
  hostInput1 = (float *)malloc(inputLengthBytes);
  hostInput2 = (float *)malloc(inputLengthBytes);
  hostOutput       = (float *)malloc(inputLengthBytes);

//#pragma omp parallel for
  for(int i=0; i<inputLength; i++)
  {
    hostInput1[i] = (float)(i % 11);
    hostInput2[i] = (float)(i % 31);
  }


#pragma omp parallel for
  for(int i=0; i<inputLength; i++)
  {
    hostOutput[i] = hostInput1[i] + hostInput2[i];
  }

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
