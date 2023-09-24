#pragma once
#include "../global_define/mat_lib.h"
#include "../global_define/global_define.h"
#include "../GISmsmt_prcs.h"
#include "GISPzx.h"
#define N_AP_HROW (N_MEAS*(N_MEAS-1)/2)
extern "C"{
void another_GISPzx(msmt* msmtinfo, Mat_S* Pxx, Mat_S* Hxx, Mat_S* z_cap, Mat_S* R_mat,
					fixed_type pzx_fp[N_MEAS*N_MEAS],fixed_type zDiff_fp[N_MEAS]);
}
//void reshape_pxx(Mat_S* pxx, fixed_type pxx_array[PXX_AP_LEN]);
void symmetric_mul(Mat_S* Hxx, int n_row, fixed_type mult_rst[PXX_AP_LEN]);
void elementwise_acc(fixed_type Hxx_combination[PXX_AP_LEN],Mat_S* pxx, fixed_type R_ele, fixed_type* acc);
void asymmetric_mul(Mat_S* Hxx, int n_row, int s_row,fixed_type mult_rst[PXX_AP_LEN]);

