#pragma once
#include "../lib/global_define.h"
#include "../lib/mat_lib.h"
#include "IS_dynamic.h"

void rk4(fixed_type R_input[NUM_VAR],
		fixed_type X[NUM_VAR],
		fixed_type rnd_data[4*NUM_VAR]);

