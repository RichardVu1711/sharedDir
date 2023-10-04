#pragma once
#include "mat_lib.h"
#include "global_define.h"
#include "GISmsmt_prcs.h"
#include "norm_func.h"
Mat_S ObsJacobian(Mat_S* prtcl, int step, msmt* msmtinfo);
fixed_type del(fixed_type var, fixed_type s1, fixed_type s2, fixed_type r1, fixed_type r2);
// fixed_type max_in_vector(float input_matrix[4]);
