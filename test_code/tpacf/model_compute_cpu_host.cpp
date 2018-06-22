#include <hStreams_source.h>
#include <hStreams_app_api.h>
#include <intel-coi/common/COIMacros_common.h>
/***************************************************************************
 *cr
 *cr            (C) Copyright 2007 The Board of Trustees of the
 *cr                        University of Illinois
 *cr                         All Rights Reserved
 *cr
 ***************************************************************************/
#include <sys/time.h>
#include <string.h>
#include <math.h>
#include <stdio.h> 

#include "model.h"

int doCompute(struct cartesian *data1, int n1, struct cartesian *data2, 
	      int n2, int doSelf, long long *data_bins, 
	      int nbins, float *binb)
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

  (hStreams_app_create_buf((float *)binb, (k+ 1)* sizeof (float )));
  (hStreams_app_create_buf((struct cartesian *)data2, (((n2)-1)+ 1)* sizeof (struct cartesian )));
  (hStreams_app_create_buf((long long *)data_bins, (max+1+ 1)* sizeof (long long )));
  int i, j, k;
  if (doSelf)
    {
      n2 = n1;
      data2 = data1;
    }
//  #pragma omp parallel for 
  for (i = 0; i < ((doSelf) ? n1-1 : n1); i++)
    {
      const register float xi = data1[i].x;
      const register float yi = data1[i].y;
      const register float zi = data1[i].z;

      (hStreams_app_xfer_memory((float *)binb, (float *)binb ,(k+ 1)* sizeof (float ), 0, HSTR_SRC_TO_SINK, NULL));
      (hStreams_app_xfer_memory((long long *)data_bins, (long long *)data_bins ,(max+1+ 1)* sizeof (long long ), 0, HSTR_SRC_TO_SINK, NULL));
      int sub_blocks = (((n2)-1)-((doSelf) ? i+1 : 0) + 1)/ 4;
      int remain_index = (((n2)-1)-((doSelf) ? i+1 : 0) + 1)% 4;
      int start_index = 0;
      int end_index = 0;
      uint64_t args[12];
      args[2] = (uint64_t) doSelf;
      args[3] = (uint64_t) i;
      args[4] = (uint64_t) n2;
      args[5] = (uint64_t) *((uint64_t *) (&xi));
      args[6] = (uint64_t) *((uint64_t *) (&yi));
      args[7] = (uint64_t) *((uint64_t *) (&zi));
      args[8] = (uint64_t) nbins;
      args[9] = (uint64_t) data2;
      args[10] = (uint64_t) binb;
      args[11] = (uint64_t) data_bins;
      hStreams_ThreadSynchronize();
      start_index = ((doSelf) ? i+1 : 0);
      for (int idx_subtask = 0; idx_subtask < 4; idx_subtask++)
      {
        args[0] = (uint64_t) start_index;
        end_index = start_index + sub_blocks;
        if (idx_subtask < remain_index)
          end_index ++;
        args[1] = (uint64_t) end_index;
        (hStreams_app_xfer_memory(&data2[start_index], &data2[start_index], (end_index - start_index) * sizeof (struct cartesian ), idx_subtask % 2, HSTR_SRC_TO_SINK, NULL));
        (hStreams_EnqueueCompute(
      			idx_subtask % 2,
      			"kernel",
      			9,
      			3,
      			args,
      			NULL,NULL,0));
        start_index = end_index;
      }
      hStreams_ThreadSynchronize();
        (hStreams_app_xfer_memory((long long *)data_bins, (long long *)data_bins ,(max+1+ 1)* sizeof (long long ), 0, HSTR_SINK_TO_SRC, NULL));
      hStreams_ThreadSynchronize();
    }
  
  return 0;
hStreams_app_fini();
}


