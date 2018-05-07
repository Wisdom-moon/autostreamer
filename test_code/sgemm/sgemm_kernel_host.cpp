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

/* 
 * Base C implementation of MM
 */

#include <iostream>


void basicSgemm( char transa, char transb, int m, int n, int k, float alpha, const float *A, int lda, const float *B, int ldb, float beta, float *C, int ldc )
{
  if ((transa != 'N') && (transa != 'n')) {
    std::cerr << "unsupported value of 'transa' in regtileSgemm()" << std::endl;
    return;
  }
  
  if ((transb != 'T') && (transb != 't')) {
    std::cerr << "unsupported value of 'transb' in regtileSgemm()" << std::endl;
    return;
  }
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

  (hStreams_app_create_buf(( float *)A, (((m)-1)+((k)-1)*lda-(0+0*lda) + 1)* sizeof (const float )));
  (hStreams_app_create_buf(( float *)B, (((n)-1)+((k)-1)*ldb-(0+0*ldb) + 1)* sizeof (const float )));
  (hStreams_app_create_buf((float *)C, (((m)-1)+((n)-1)*ldc-(0+0*ldc) + 1)* sizeof (float )));
  (hStreams_app_xfer_memory(( float *)A, ( float *)A ,(((m)-1)+((k)-1)*lda-(0+0*lda) + 1)* sizeof (const float ), 0, HSTR_SRC_TO_SINK, NULL));
  (hStreams_app_xfer_memory(( float *)B, ( float *)B ,(((n)-1)+((k)-1)*ldb-(0+0*ldb) + 1)* sizeof (const float ), 0, HSTR_SRC_TO_SINK, NULL));
  (hStreams_app_xfer_memory((float *)C, (float *)C ,(((m)-1)+((n)-1)*ldc-(0+0*ldc) + 1)* sizeof (float ), 0, HSTR_SRC_TO_SINK, NULL));
  int sub_blocks = m/ 4;
  int remain_index = m% 4;
  int start_index = 0;
  int end_index = 0;
  uint64_t args[13];
  args[2] = (uint64_t) m;
  args[3] = (uint64_t) n;
  args[4] = (uint64_t) k;
  args[5] = (uint64_t) lda;
  args[6] = (uint64_t) ldb;
  args[7] = (uint64_t) ldc;
  args[8] = (uint64_t) beta;
  args[9] = (uint64_t) alpha;
  args[10] = (uint64_t) A;
  args[11] = (uint64_t) B;
  args[12] = (uint64_t) C;
  hStreams_ThreadSynchronize();
  start_index = 0;
  for (int i = 0; i < 4; i++)
  {
    args[0] = (uint64_t) start_index;
    end_index = start_index + sub_blocks;
    if (i < remain_index)
      end_index ++;
    args[1] = (uint64_t) end_index;
    (hStreams_EnqueueCompute(
  			i % 2,
  			"kernel",
  			10,
  			3,
  			args,
  			NULL,NULL,0));
    start_index = end_index;
  }
  hStreams_ThreadSynchronize();
    (hStreams_app_xfer_memory((float *)C, (float *)C ,(((m)-1)+((n)-1)*ldc-(0+0*ldc) + 1)* sizeof (float ), 0, HSTR_SINK_TO_SRC, NULL));
  hStreams_app_fini();
}

