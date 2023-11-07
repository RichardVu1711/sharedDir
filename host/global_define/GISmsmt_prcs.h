#pragma once
#include "mat_lib.h"
#include "global_define.h"

#define SN_NUM 3
#define AOASTD 1.0
#define TDOASTD 7.0
#define OL_THOLD 3.0
typedef struct msmt{
	fixed_type z[N_MEAS];
	int n_aoa;
	int n_tdoa;
	int aoaIdx[N_AOA];
	int tdoaIdx[N_TDOA];
	int validIdx[N_MEAS];
} msmt;


fixed_type deg2Rad(fixed_type deg);
msmt msmt_prcs(Mat_S* obsVals, int index, fixed_type cAvg[N_MEAS], fixed_type nAvg[N_MEAS]);
void outlier_detector(Mat_S* obsVals,fixed_type cAvg[N_MEAS], int index, fixed_type nAvg[N_MEAS], fixed_type fil_data[N_MEAS]);
