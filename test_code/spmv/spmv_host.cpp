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

#include <parboil.h>
#include <stdio.h>
#include <stdlib.h>

#include "file.h"
#include "convert_dataset.h"

static int generate_vector(float *x_vector, int dim) 
{	
	srand(54321);	
  int i;
	for(i=0;i<dim;i++)
	{
		x_vector[i] = (rand() / (float) RAND_MAX);
	}
	return 0;
}

/*
void jdsmv(int height, int len, float* value, int* perm, int* jds_ptr, int* col_index, float* vector,
        float* result){
        int i;
        int col,row;
        int row_index =0;
        int prem_indicator=0;
        for (i=0; i<len; i++){
                if (i>=jds_ptr[prem_indicator+1]){
                        prem_indicator++;
                        row_index=0;
                }
                if (row_index<height){
                col = col_index[i];
                row = perm[row_index];
                result[row]+=value[i]*vector[col];
                }

                row_index++;
        }
        return;
}
*/
int main(int argc, char** argv) {
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

	struct pb_TimerSet timers;
	struct pb_Parameters *parameters;
	
	
	
	
	
	printf("CPU-based sparse matrix vector multiplication****\n");
	printf("Original version by Li-Wen Chang <lchang20@illinois.edu> and Shengzhao Wu<wu14@illinois.edu>\n");
	printf("This version maintained by Chris Rodrigues  ***********\n");
	parameters = pb_ReadParameters(&argc, argv);
	if ((parameters->inpFiles[0] == NULL) || (parameters->inpFiles[1] == NULL))
    {
      fprintf(stderr, "Expecting two input filenames\n");
      exit(-1);
    }

	
	pb_InitializeTimerSet(&timers);
	pb_SwitchToTimer(&timers, pb_TimerID_COMPUTE);
	
	//parameters declaration
	int len;
	int depth;
	int dim;
	int pad=1;
	int nzcnt_len;
	
	//host memory allocation
	//matrix
	float *h_data;
	int *h_indices;
	int *h_ptr;
	int *h_perm;
	int *h_nzcnt;
	//vector
	float *h_Ax_vector;
    (hStreams_app_create_buf((float *)h_Ax_vector, (sizeof(float)*d)));
    (hStreams_app_create_buf((float *)h_data, (j+ 1)* sizeof (float )));
    (hStreams_app_create_buf((int *)h_indices, (j+ 1)* sizeof (int )));
    (hStreams_app_create_buf((int *)h_nzcnt, (((dim)-1)+ 1)* sizeof (int )));
    (hStreams_app_create_buf((int *)h_ptr, (((bound)-1)+ 1)* sizeof (int )));
    (hStreams_app_create_buf((float *)h_x_vector, (sizeof(float)*d)));
    float *h_x_vector;
	
	
    //load matrix from files
	pb_SwitchToTimer(&timers, pb_TimerID_IO);
	//inputData(parameters->inpFiles[0], &len, &depth, &dim,&nzcnt_len,&pad,
	//    &h_data, &h_indices, &h_ptr,
	//    &h_perm, &h_nzcnt);

 

	int col_count;
	coo_to_jds(
		parameters->inpFiles[0], // bcsstk32.mtx, fidapm05.mtx, jgl009.mtx
		1, // row padding
		pad, // warp size
		1, // pack size
		1, // is mirrored?
		0, // binary matrix
		1, // debug level [0:2]
		&h_data, &h_ptr, &h_nzcnt, &h_indices, &h_perm,
		&col_count, &dim, &len, &nzcnt_len, &depth
	);		

  h_Ax_vector=(float*)malloc(sizeof(float)*dim);
  h_x_vector=(float*)malloc(sizeof(float)*dim);
//  generate_vector(h_x_vector, dim);
  input_vec( parameters->inpFiles[1],h_x_vector,dim);

	
	pb_SwitchToTimer(&timers, pb_TimerID_COMPUTE);

	
  int p, i;
	//main execution
	for(p=0;p<50;p++)
	{
    (hStreams_app_xfer_memory((float *)h_data, (float *)h_data ,(j+ 1)* sizeof (float ), 0, HSTR_SRC_TO_SINK, NULL));
    (hStreams_app_xfer_memory((int *)h_indices, (int *)h_indices ,(j+ 1)* sizeof (int ), 0, HSTR_SRC_TO_SINK, NULL));
    (hStreams_app_xfer_memory((int *)h_ptr, (int *)h_ptr ,(((bound)-1)+ 1)* sizeof (int ), 0, HSTR_SRC_TO_SINK, NULL));
    (hStreams_app_xfer_memory((float *)h_x_vector, (float *)h_x_vector ,(sizeof(float)*d), 0, HSTR_SRC_TO_SINK, NULL));
    int sub_blocks = (((dim)-1)-0 + 1)/ 4;
    int remain_index = (((dim)-1)-0 + 1)% 4;
    int start_index = 0;
    int end_index = 0;
    uint64_t args[10];
    args[2] = (uint64_t) dim;
    args[3] = (uint64_t) h_nzcnt;
    args[4] = (uint64_t) h_ptr;
    args[5] = (uint64_t) h_indices;
    args[6] = (uint64_t) h_data;
    args[7] = (uint64_t) h_x_vector;
    args[8] = (uint64_t) h_Ax_vector;
    args[9] = (uint64_t) h_perm;
    hStreams_ThreadSynchronize();
    start_index = 0;
    for (int idx_subtask = 0; idx_subtask < 4; idx_subtask++)
    {
      args[0] = (uint64_t) start_index;
      end_index = start_index + sub_blocks;
      if (idx_subtask < remain_index)
        end_index ++;
      args[1] = (uint64_t) end_index;
      (hStreams_app_xfer_memory(&h_nzcnt[start_index], &h_nzcnt[start_index], (end_index - start_index) * sizeof (int ), idx_subtask % 2, HSTR_SRC_TO_SINK, NULL));
      (hStreams_EnqueueCompute(
    			idx_subtask % 2,
    			"kernel",
    			3,
    			7,
    			args,
    			NULL,NULL,0));
      (hStreams_app_xfer_memory(&h_Ax_vector[start_index], &h_Ax_vector[start_index], (end_index - start_index) * sizeof (float ), idx_subtask % 2, HSTR_SINK_TO_SRC, NULL));
      start_index = end_index;
    }
    hStreams_ThreadSynchronize();
	}	

	if (parameters->outFile) {
		pb_SwitchToTimer(&timers, pb_TimerID_IO);
		outputData(parameters->outFile,h_Ax_vector,dim);
		
	}
	pb_SwitchToTimer(&timers, pb_TimerID_COMPUTE);
	
	free (h_data);
	free (h_indices);
	free (h_ptr);
	free (h_perm);
	free (h_nzcnt);
	free (h_Ax_vector);
	free (h_x_vector);
	pb_SwitchToTimer(&timers, pb_TimerID_NONE);

	pb_PrintTimerSet(&timers);
	pb_FreeParameters(parameters);

	return 0;

hStreams_app_fini();
}

