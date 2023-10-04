#pragma once
#include "ESPCrtParticles.h"
#include "Calweights.h"
#include "resample_pf.h"
#include "PFupdate.h"

extern "C"
{
void ESP_PF_Wrapper(Mat_S* obs_FP,Mat_S* pxx_FP,Mat_S* state_FP, fixed_type wt[NUM_PARTICLES], Mat* prtcls_in,
					Mat_S* stateOut, Mat_S* pxxOut, Mat* prtcl_out, fixed_type wtOut[NUM_PARTICLES],
					int step);
}
void ESP_PF(Mat_S* obs_FP,Mat_S* Pxx_FP,Mat_S* state_FP, fixed_type wt[NUM_PARTICLES], Mat* prtcls_act, int step);
