#pragma once
#include "../global_define/mat_lib.h"
#include "../global_define/global_define.h"
#include "../GISmsmt_prcs.h"
#define N_PZX N_MEAS*(N_MEAS+1)/2
#define N_SYM N_MEAS
#define N_ASYM N_MEAS*(N_MEAS-1)/2
#define BIT_LEN 8
//extern "C"{
void GISPzx(msmt* msmtinfo, fixed_type Pxx[NUM_VAR*NUM_VAR], fixed_type Hxx[NUM_VAR*NUM_VAR], fixed_type z_cap[NUM_VAR*NUM_VAR],
			fixed_type pzx_fp[N_MEAS][N_MEAS],fixed_type zDiff_fp[N_MEAS]);
//}
void symmetric_mul(fixed_type Hxx[],  fixed_type mult_rst[PXX_AP_LEN*N_SYM]);
void asymmetric_mul(fixed_type Hxx[], fixed_type mult_rst[PXX_AP_LEN*N_ASYM]);
void elewise_asymAcc(fixed_type Hxx_combination[PXX_AP_LEN*N_ASYM],fixed_type pxx[NUM_VAR*NUM_VAR], fixed_type acc[N_ASYM]);
void elewise_symAcc(fixed_type Hxx_combination[PXX_AP_LEN*N_SYM],fixed_type pxx[NUM_VAR*NUM_VAR], Mat_S* R_ele, fixed_type acc[N_SYM]);
void cp_pzx(fixed_type acc_sym[N_SYM], fixed_type acc_asym[N_ASYM], fixed_type pzx[N_MEAS][N_MEAS], msmt* msmtinfo, Mat_S* z_cap, fixed_type zDiff[N_MEAS] );


//void copy_resultPzx(fixed_type val, int idx1, int idx2, fixed_type pzx_fp[N_MEAS][N_MEAS]);
void idx_extract( int i, int* idx1, int* idx2);

void pzx_cal(fixed_type Hxx_local[NUM_VAR*NUM_VAR], fixed_type pxx_local[NUM_VAR*NUM_VAR], Mat_S* R_mat,
			fixed_type pzx_fp[N_MEAS][N_MEAS]);
void duplicate_pzxDF(fixed_type  hxx_local[NUM_VAR*NUM_VAR],fixed_type  pxx_local[NUM_VAR*NUM_VAR],
				fixed_type Hxx_local1[N_MEAS*NUM_VAR],fixed_type Hxx_local2[N_MEAS*NUM_VAR],
				fixed_type pxx_local1[NUM_VAR*NUM_VAR], fixed_type pxx_local2[NUM_VAR*NUM_VAR]);
Mat_S R_cal(int n_aoa, int n_tdoa);
