#pragma once
#include "ObsJacobian.h"
#include "GISPzx.h"
#include "GISobs_model.h"
#include <stdio.h>
#include <string.h>
extern "C"
{
void CalPzxZdiff(Mat* prtcls, msmt* msmtinfo1,msmt* msmtinfo2,msmt* msmtinfo3,
				int index,
				Mat_S* Pxx_,
				fixed_type zDiff[NUM_PARTICLES*SN_NUM*2],
				fixed_type pzx[NUM_PARTICLES*SN_NUM*2*SN_NUM*2]);
}
extern "C"
{
void mean_X_and_Pxx(Mat* prtcls, Mat_S* Pxx_);
}
void pxx_calAvg(Mat* particle, Mat_S* state, fixed_type acc [NUM_VAR*(NUM_VAR+1)/2]);
void avg_selPrtcls(fixed_type prtcls[], int col, Mat_S* X_avg, fixed_type sub[NUM_VAR]);
void avg_mul(fixed_type sub1[],fixed_type sub2[], fixed_type mul[]);
void avg_acc(fixed_type mul[], fixed_type acc[]);

void duplicate_data(Mat* in, int sel, Mat_S* out1, Mat_S* out2);
void copy_result(	fixed_type pzx_fp[SN_NUM*2][SN_NUM*2],
					fixed_type zDiff_fp[SN_NUM*2],
					int j,
					fixed_type zDiff[NUM_PARTICLES*SN_NUM*2],
					fixed_type pzx[NUM_PARTICLES*SN_NUM*2*SN_NUM*2]);
