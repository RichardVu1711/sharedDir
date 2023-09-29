#pragma once
#include "mat_lib.h"
#include "global_define.h"

#define SN_NUM 3
typedef struct msmt{
	Mat_S z;
	int n_aoa;
	int n_tdoa;
	int aoaIdx[3];
	int tdoaIdx[3];
	int validIdx[N_MEAS];
} msmt;


msmt msmt_prcs(Mat_S* obsVals);
fixed_type deg2Rad(fixed_type deg);
