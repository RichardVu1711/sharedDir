#pragma once
#include "global_define.h"
#include "mat_lib.h"


void PFupdate(Mat* particle, fixed_type wt[NUM_PARTICLES], Mat_S* pxx,Mat_S* state);
void Up_selPrtcls(fixed_type prtcls[], int col, Mat_S* X_avg, Mat_S* temp_X2);
void Up_sumAcc(fixed_type currPxx[],fixed_type newCov[], fixed_type wt,fixed_type outMat[]);
void Up_loadPxx(fixed_type inMat[], fixed_type outMat[]);
void Up_mulColRow(fixed_type inMat[], fixed_type outMat[]);
