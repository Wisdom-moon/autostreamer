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

#include <math.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#define PI   3.1415926535897932384626433832795029f
#define PIx2 6.2831853071795864769252867665590058f

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define K_ELEMS_PER_GRID 2048

struct kValues {
  float Kx;
  float Ky;
  float Kz;
  float PhiMag;
};

inline
void 
ComputePhiMagCPU(int numK, 
                 float* phiR, float* phiI, float* phiMag) {
  int indexK = 0;
//  #pragma omp parallel for
  for (indexK = 0; indexK < numK; indexK++) {
    float real = phiR[indexK];
    float imag = phiI[indexK];
    phiMag[indexK] = real*real + imag*imag;
  }
}

inline
void
ComputeQCPU(int numK, int numX,
            struct kValues *kVals,
            float* x, float* y, float* z,
            float *Qr, float *Qi) {
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

  (hStreams_app_create_buf((float *)Qi, (((numX)-1)+ 1)* sizeof (float )));
  (hStreams_app_create_buf((float *)Qr, (((numX)-1)+ 1)* sizeof (float )));
  (hStreams_app_create_buf((struct kValues *)kVals, (((numK)-1)+ 1)* sizeof (struct kValues )));
  (hStreams_app_create_buf((float *)x, (((numX)-1)+ 1)* sizeof (float )));
  (hStreams_app_create_buf((float *)y, (((numX)-1)+ 1)* sizeof (float )));
  (hStreams_app_create_buf((float *)z, (((numX)-1)+ 1)* sizeof (float )));
  float expArg;
  float cosArg;
  float sinArg;

  int indexK, indexX;
  (hStreams_app_xfer_memory((float *)Qi, (float *)Qi ,(((numX)-1)+ 1)* sizeof (float ), 0, HSTR_SRC_TO_SINK, NULL));
  (hStreams_app_xfer_memory((float *)Qr, (float *)Qr ,(((numX)-1)+ 1)* sizeof (float ), 0, HSTR_SRC_TO_SINK, NULL));
  (hStreams_app_xfer_memory((float *)x, (float *)x ,(((numX)-1)+ 1)* sizeof (float ), 0, HSTR_SRC_TO_SINK, NULL));
  (hStreams_app_xfer_memory((float *)y, (float *)y ,(((numX)-1)+ 1)* sizeof (float ), 0, HSTR_SRC_TO_SINK, NULL));
  (hStreams_app_xfer_memory((float *)z, (float *)z ,(((numX)-1)+ 1)* sizeof (float ), 0, HSTR_SRC_TO_SINK, NULL));
  int sub_blocks = (((numK)-1)-0 + 1)/ 4;
  int remain_index = (((numK)-1)-0 + 1)% 4;
  int start_index = 0;
  int end_index = 0;
  uint64_t args[10];
  args[2] = (uint64_t) numK;
  args[3] = (uint64_t) numX;
  args[4] = (uint64_t) kVals;
  args[5] = (uint64_t) x;
  args[6] = (uint64_t) y;
  args[7] = (uint64_t) z;
  args[8] = (uint64_t) Qr;
  args[9] = (uint64_t) Qi;
  hStreams_ThreadSynchronize();
  start_index = 0;
  for (int idx_subtask = 0; idx_subtask < 4; idx_subtask++)
  {
    args[0] = (uint64_t) start_index;
    end_index = start_index + sub_blocks;
    if (idx_subtask < remain_index)
      end_index ++;
    args[1] = (uint64_t) end_index;
    (hStreams_app_xfer_memory(&kVals[start_index], &kVals[start_index], (end_index - start_index) * sizeof (struct kValues ), idx_subtask % 2, HSTR_SRC_TO_SINK, NULL));
    (hStreams_EnqueueCompute(
  			idx_subtask % 2,
  			"kernel",
  			4,
  			6,
  			args,
  			NULL,NULL,0));
    start_index = end_index;
  }
  hStreams_ThreadSynchronize();
    (hStreams_app_xfer_memory((float *)Qi, (float *)Qi ,(((numX)-1)+ 1)* sizeof (float ), 0, HSTR_SINK_TO_SRC, NULL));
    (hStreams_app_xfer_memory((float *)Qr, (float *)Qr ,(((numX)-1)+ 1)* sizeof (float ), 0, HSTR_SINK_TO_SRC, NULL));
  hStreams_ThreadSynchronize();
hStreams_app_fini();
}

void createDataStructsCPU(int numK, int numX, float** phiMag,
	 float** Qr, float** Qi)
{
  *phiMag = (float* ) memalign(16, numK * sizeof(float));
  *Qr = (float*) memalign(16, numX * sizeof (float));
  memset((void *)*Qr, 0, numX * sizeof(float));
  *Qi = (float*) memalign(16, numX * sizeof (float));
  memset((void *)*Qi, 0, numX * sizeof(float));
}

