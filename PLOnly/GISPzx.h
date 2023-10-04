#pragma once
#include "mat_lib.h"
#include "global_define.h"
#include "GISmsmt_prcs.h"
#include "mvnpdf_code.h"
fixed_type GISPzx(msmt* msmtinfo, Mat_S* Pxx, Mat_S* Hxx, Mat_S* z_cap);
Mat_S R_cal(msmt* msmtinfo);
