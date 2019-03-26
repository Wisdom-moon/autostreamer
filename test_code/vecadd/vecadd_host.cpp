#include <hStreams_source.h>
#include <hStreams_app_api.h>
#include <intel-coi/common/COIMacros_common.h>
#include <omp.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  uint32_t logical_streams_per_place= 1;
  uint32_t places_per_domain = 2;
  HSTR_OPTIONS hstreams_options;

  hStreams_GetCurrentOptions(&hstreams_options, sizeof(hstreams_options));
  hstreams_options.verbose = 0;
  hstreams_options.phys_domains_limit = 256;
  char *libNames[20] = {NULL,NULL};
  unsigned int libNameCnt = 0;
  libNames[libNameCnt++] = "kernel.so";
  hstreams_options.libNames = libNames;
  hstreams_options.libNameCnt = (uint16_t)libNameCnt;
  hStreams_SetOptions(&hstreams_options);

  int iret = hStreams_app_init(places_per_domain, logical_streams_per_place);
  if( iret != 0 )
  {
    printf("hstreams_app_init failed!\n");
    exit(-1);
  }

  int inputLength = 32;
  int inputLengthBytes;
  float *hostInput1;
  float *hostInput2;
  (hStreams_app_create_buf((float *)hostInput1, (inputLengthBytes)));
  (hStreams_app_create_buf((float *)hostInput2, (inputLengthBytes)));
  (hStreams_app_create_buf((float *)hostOutput, (inputLengthBytes)));
  float *hostOutput;

  if (argc > 1) {
    inputLength = atoi (argv[1]);
  }

  inputLengthBytes = inputLength * sizeof(float);
  hostInput1 = (float *)malloc(inputLengthBytes);
  hostInput2 = (float *)malloc(inputLengthBytes);
  hostOutput       = (float *)malloc(inputLengthBytes);

  for(int i=0; i<inputLength; i++)
  {
    hostInput1[i] = (float)(i % 11);
    hostInput2[i] = (float)(i % 31);
  }


int sub_blocks = (((inputLength)-1)-0 + 1)/ 4;
int remain_index = (((inputLength)-1)-0 + 1)% 4;
int start_index = 0;
int end_index = 0;
uint64_t args[6];
args[2] = (uint64_t) inputLength;
args[3] = (uint64_t) hostOutput;
args[4] = (uint64_t) hostInput1;
args[5] = (uint64_t) hostInput2;
hStreams_ThreadSynchronize();
start_index = 0;
for (int idx_subtask = 0; idx_subtask < 4; idx_subtask++)
{
  args[0] = (uint64_t) start_index;
  end_index = start_index + sub_blocks;
  if (idx_subtask < remain_index)
    end_index ++;
  args[1] = (uint64_t) end_index;
  (hStreams_app_xfer_memory(&hostInput1[start_index], &hostInput1[start_index], (end_index - start_index) * sizeof (float ), idx_subtask % 2, HSTR_SRC_TO_SINK, NULL));
  (hStreams_app_xfer_memory(&hostInput2[start_index], &hostInput2[start_index], (end_index - start_index) * sizeof (float ), idx_subtask % 2, HSTR_SRC_TO_SINK, NULL));
  (hStreams_EnqueueCompute(
			idx_subtask % 2,
			"kernel",
			3,
			3,
			args,
			NULL,NULL,0));
  (hStreams_app_xfer_memory(&hostOutput[start_index], &hostOutput[start_index], (end_index - start_index) * sizeof (float ), idx_subtask % 2, HSTR_SINK_TO_SRC, NULL));
  start_index = end_index;
}
hStreams_ThreadSynchronize();

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
hStreams_app_fini();
}

