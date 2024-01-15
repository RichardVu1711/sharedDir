#pragma once
#include "../global_define/global_define.h"
#include "../global_define/mat_lib.h"
#include "IS_dynamic.h"

void rk4(fixed_type R_input[NUM_VAR],
		fixed_type X[NUM_VAR],
		fixed_type rnd_data[4*NUM_VAR]);

