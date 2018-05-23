#include <hStreams_source.h>
#include <hStreams_app_api.h>
#include <intel-coi/common/COIMacros_common.h>
/***************************************************************************
 *cr
 *cr            (C) Copyright 2010 The Board of Trustees of the
 *cr                        University of Illinois
 *cr                         All Rights Reserved
 *cr
 ***************************************************************************/

#include "common.h"

void cpu_stencil(float c0,float c1, float *A0,float * Anext,const int nx, const int ny, const int nz)
{

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

  (hStreams_app_create_buf((float *)A0, (((((nx-1)-1))+nx*((((ny-1)-1))+ny*(((nz-1)-1)+1)))+ 1)* sizeof (float )));
  (hStreams_app_create_buf((float *)Anext, (((((nx-1)-1))+nx*((((ny-1)-1))+ny*(((nz-1)-1))))+ 1)* sizeof (float )));
  int i;  
  int sub_blocks = (((nx-1)-1)-1 + 1)/ 4;
  int remain_index = (((nx-1)-1)-1 + 1)% 4;
  int start_index = 0;
  int end_index = 0;
  uint64_t args[9];
  args[2] = (uint64_t) nx;
  args[3] = (uint64_t) ny;
  args[4] = (uint64_t) nz;
  args[5] = (uint64_t) *((uint64_t *) (&c1));
  args[6] = (uint64_t) *((uint64_t *) (&c0));
  args[7] = (uint64_t) Anext;
  args[8] = (uint64_t) A0;
  hStreams_ThreadSynchronize();
  start_index = 1;
  for (int idx_subtask = 0; idx_subtask < 4; idx_subtask++)
  {
    args[0] = (uint64_t) start_index;
    end_index = start_index + sub_blocks;
    if (idx_subtask < remain_index)
      end_index ++;
    args[1] = (uint64_t) end_index;
    (hStreams_app_xfer_memory(&A0[start_index], &A0[start_index], (end_index - start_index) * sizeof (float ), idx_subtask % 2, HSTR_SRC_TO_SINK, NULL));
    (hStreams_EnqueueCompute(
  			idx_subtask % 2,
  			"kernel",
  			7,
  			2,
  			args,
  			NULL,NULL,0));
    (hStreams_app_xfer_memory(&Anext[start_index], &Anext[start_index], (end_index - start_index) * sizeof (float ), idx_subtask % 2, HSTR_SINK_TO_SRC, NULL));
    start_index = end_index;
  }
  hStreams_ThreadSynchronize();

hStreams_app_fini();
}



