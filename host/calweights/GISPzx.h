#pragma once
#include "../global_define/mat_lib.h"
#include "../global_define/global_define.h"
#include "../global_define/GISmsmt_prcs.h"
#include "hls_stream.h"

#define N_PZX N_MEAS*(N_MEAS+1)/2
#define N_SYM N_MEAS
#define N_ASYM N_MEAS*(N_MEAS-1)/2
#define N_MM 4
#define BIT_LEN 8

void zDiff_cal(fixed_type z_cap[N_MEAS],
			msmt* msmtinfo,
			fixed_type zDiff_local[NUM_PARTICLES*N_MEAS],
			int n_obs, int idx);

void s_macc3_13(fixed_type temp1[NUM_VAR],
				fixed_type temp2[NUM_VAR],
				fixed_type out[1]);
void GISPzx3(fixed_type Hxx_local[N_MEAS][NUM_VAR], fixed_type pxx_local[NUM_VAR][NUM_VAR], Mat_S* R_mat,
		fixed_type pzx_local[NUM_PARTICLES*N_MEAS*N_MEAS], int n_obs, int idx);
Mat_S R_cal(int n_aoa, int n_tdoa);
