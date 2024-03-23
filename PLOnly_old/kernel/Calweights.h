#pragma once
#include "ObsJacobian.h"
#include "GISPzx.h"
#include "GISobs_model.h"
void Calweights(fixed_type wt[NUM_PARTICLES], Mat* particle, Mat_S* obsVals,int index);
void mean_X_and_Pxx(Mat* prtcls, Mat_S* Pxx_);
void avg_cal(Mat* prtcls, Mat_S* X_avg, Mat_S* Pxx_);
void selPrtcls(fixed_type prtcls[], int col, Mat_S* X_avg, Mat_S* temp_X2);
void sumAcc(fixed_type currPxx[],fixed_type newCov[], fixed_type outMat[]);
void loadPxx(fixed_type inMat[], fixed_type outMat[]);
void mulColRow(fixed_type inMat[], fixed_type outMat[]);

