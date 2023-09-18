#pragma once
#include "norm_func.h"
#include "../lib/mat_lib.h"
#include "../lib/global_define.h"
#include "../lib/GISmsmt_prcs.h"
void ObsJacobian(Mat_S* prtcl_X, int step, msmt* msmtinfo, Mat_S* H);
fixed_type del(fixed_type var, fixed_type s1, fixed_type s2, fixed_type r1, fixed_type r2);
// fixed_type max_in_vector(float input_matrix[4]);
