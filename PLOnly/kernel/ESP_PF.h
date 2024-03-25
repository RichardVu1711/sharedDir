#pragma once
#include "ESPCrtParticles.h"
#include "Calweights.h"
#include "resample_pf/resample_pf.h"
#include "PFupdate.h"
#include "rk4/rk4.h"
#include "resample_pf/resample_pf.h"
#include "random_generator/RNG_withSeed.h"
#include "calW/mvnpdf_code.h"
#include "mean_pxx.h"
#include "sigmaComp.h"
//#include "lib/read_write_csv.h"

extern "C"
{
void ESP_PF_Wrapper(Mat_S* obs,fixed_type pxx_in[NUM_VAR*NUM_VAR],fixed_type state_In[NUM_VAR], 
					fixed_type wt[NUM_PARTICLES],
					fixed_type pxxOut[NUM_VAR*NUM_VAR],
					fixed_type stateOut[NUM_VAR], 
					fixed_type wtOut[NUM_PARTICLES],
					int step, int seed);
}
void ESP_PF(Mat_S* obs,fixed_type pxx[NUM_VAR],fixed_type state[NUM_VAR], 
			fixed_type wt[NUM_PARTICLES], 
			int step, int seed);
