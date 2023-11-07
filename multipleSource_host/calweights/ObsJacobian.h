#pragma once
#include "norm_func.h"
#include "../global_define/mat_lib.h"
#include "../global_define/global_define.h"
#include "../global_define/GISmsmt_prcs.h"

void ObsJacobian(Mat_S* prtcl_X, int step, msmt* msmtinfo, fixed_type H[N_MEAS][NUM_VAR]);
fixed_type del(fixed_type var, fixed_type s1, fixed_type s2, fixed_type r1, fixed_type r2);
// fixed_type max_in_vector(float input_matrix[4]);
