#pragma once
#include <stdio.h>
#include <string.h>

#include "calW/GISobs_model.h"
#include "calW/GISPzx.h"
#include "calW/ObsJacobian.h"

void duplicate_data(Mat* in, int sel, Mat_S* out1, Mat_S* out2);
void copy_result(	fixed_type pzx_fp[SN_NUM*2][SN_NUM*2],
					fixed_type zDiff_fp[SN_NUM*2],
					int j,
					fixed_type zDiff[NUM_PARTICLES*SN_NUM*2],
					fixed_type pzx[NUM_PARTICLES*SN_NUM*2*SN_NUM*2]);
extern "C"
{
void CalPzxZdiff(fixed_type prtcls[NUM_VAR*NUM_PARTICLES],
				msmt* msmtinfo,
				fixed_type R [N_MEAS],
				int index,
				fixed_type Pxx_[NUM_VAR*NUM_VAR],
				fixed_type zDiff[NUM_PARTICLES*N_MEAS],
				fixed_type pzx[NUM_PARTICLES*N_MEAS*N_MEAS]);
}
void load_data(fixed_type pxx[NUM_VAR*NUM_VAR],
				fixed_type prtcls [NUM_VAR*NUM_PARTICLES],
				fixed_type R [N_MEAS],
				msmt* msmtinfo,
				fixed_type pxx_Out1[NUM_VAR*NUM_VAR],
				fixed_type pxx_Out2[NUM_VAR*NUM_VAR],
				Mat* prtcls_temp,
				Mat_S* R_out,
				msmt* msmtinfo1,
				msmt* msmtinfo2,
				msmt* msmtinfo3);

void store_data( fixed_type zDiff_local[NUM_PARTICLES*SN_NUM*2],
				fixed_type pzx_local[NUM_PARTICLES*SN_NUM*2*SN_NUM*2],
				fixed_type zDiff[NUM_PARTICLES*SN_NUM*2],
				fixed_type pzx[NUM_PARTICLES*SN_NUM*2*SN_NUM*2]);

