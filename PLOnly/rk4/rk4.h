#pragma once
#include "../lib/global_define.h"
#include "../lib/mat_lib.h"
#include "IS_dynamic.h"

void rk4(Mat_S* R_input,Mat_S* X,fixed_type rnd_data[4*NUM_VAR]);

