#pragma once
#include "../lib/global_define.h"
#include "../lib/mat_lib.h"

void IS_dynamics(const fixed_type X[NUM_VAR], fixed_type dX[NUM_VAR], fixed_type rnd_data[NUM_VAR]);
