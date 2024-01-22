#pragma once
#include "../lib/mat_lib.h"
#include "../lib/global_define.h"

#define SN_NUM 3
typedef struct msmt{
	fixed_type z[N_MEAS];
	int n_aoa;
	int n_tdoa;
	int aoaIdx[N_AOA];
	int tdoaIdx[N_TDOA];
	int validIdx[N_MEAS];
} msmt;


msmt msmt_prcs(fixed_type obsVals[]);
fixed_type deg2Rad(fixed_type deg);
void R_cal(int n_aoa, int n_tdoa, fixed_type R_noise[N_MEAS]);
