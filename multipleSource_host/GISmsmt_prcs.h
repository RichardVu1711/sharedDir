#pragma once
#include "global_define/mat_lib.h"
#include "global_define/global_define.h"

#define SN_NUM 3
typedef struct msmt{
	Mat_S z;
	int n_aoa;
	int n_tdoa;
	int aoaIdx[3];
	int tdoaIdx[3];
} msmt;


msmt msmt_prcs(fixed_type obsVals[10]);
fixed_type deg2Rad(fixed_type deg);
