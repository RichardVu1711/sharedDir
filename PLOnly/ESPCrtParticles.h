#pragma once
#include "rk4.h"
#include "normrnd.h"

void ESPCrtParticles(Mat_S* state_input, Mat_S* Pxx_input, Mat* particles,
		fixed_type rnd_signma[NUM_PARTICLES*NUM_VAR],fixed_type rnd_rk4[4*NUM_VAR]);
// Mat ESPCrtParticles(Mat_S* state_input, Mat_S* Pxx_input, Mat_S* X_pro, int Ns, int dt);
