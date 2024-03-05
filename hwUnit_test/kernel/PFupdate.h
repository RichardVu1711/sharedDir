#pragma once
#include "lib/global_define.h"
#include "lib/mat_lib.h"
#include <stdio.h>
#include <string.h>

extern "C"{
void PFupdate(fixed_type particle[NUM_VAR*NUM_PARTICLES],
			fixed_type wt[NUM_PARTICLES],
			fixed_type state[NUM_VAR],
			fixed_type pxx[NUM_VAR*NUM_VAR]);
}
void Up_selPrtcls(fixed_type prtcls[], int col, Mat_S* X_avg, fixed_type sub[NUM_VAR]);
void Up_mul(fixed_type sub1[],fixed_type sub2[], fixed_type mul[]);
void Up_acc(fixed_type mul[], fixed_type wt, fixed_type acc[]);
void pxx_cal(Mat* particle, Mat_S* state,fixed_type wt[], fixed_type acc [NUM_VAR*(NUM_VAR+1)/2]);
void store_PFU_data(Mat_S* state_local, Mat_S* pxx_local,
					fixed_type state[NUM_VAR],
					fixed_type pxx[NUM_VAR*NUM_VAR]);
void load_PFU_data(fixed_type prtcls[NUM_VAR*NUM_PARTICLES],
					fixed_type wt[NUM_PARTICLES],
					Mat* prtcls_local,
					fixed_type wt_local[NUM_PARTICLES]);
