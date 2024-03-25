#pragma once
#include "lib/mat_lib.h"
#include "lib/global_define.h"
#include <stdio.h>
#include <string.h>

extern "C"
{
// unroll mean calculation
void mean_Pxx(fp_str& prtcls,
			fixed_type Pxx_[NUM_VAR*NUM_VAR]);
}
void pxx_calAvg(Mat* particle, Mat_S* state, fixed_type acc [NUM_VAR*(NUM_VAR+1)/2]);
void avg_selPrtcls(fixed_type prtcls[], int col, Mat_S* X_avg, fixed_type sub[NUM_VAR]);
void avg_mul(fixed_type sub1[],fixed_type sub2[], fixed_type mul[]);
void avg_acc(fixed_type mul[], fixed_type acc[]);
