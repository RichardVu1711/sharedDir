#pragma once
#include "../lib/global_define.h"
#include "../lib/mat_lib.h"

void resamplePF_wrap(fixed_type particle_in [NUM_VAR*NUM_PARTICLES],
					fixed_type wt[NUM_PARTICLES], fixed_type r);

int cumsum(fixed_type weights[NUM_PARTICLES],fixed_type edges[NUM_PARTICLES+1]);
int bin_created(fixed_type start, fixed_type step, fixed_type end, fixed_type boundary[NUM_PARTICLES]);
int Which_bin(fixed_type data, fixed_type * bin_arr, int bin_count, fixed_type start_meas);

int resample_pf(fixed_type particle_in [NUM_VAR*NUM_PARTICLES],
				fixed_type weights_in[NUM_PARTICLES], int Ns, fixed_type r,
				fixed_type weights_out[NUM_PARTICLES]);
