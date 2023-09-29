#pragma once
#include "../global_define/mat_lib.h"
#include "../global_define/global_define.h"
#include "../global_define/GISmsmt_prcs.h"
extern "C"{
void GISPzx(msmt* msmtinfo, Mat_S* Pxx, Mat_S* Hxx, Mat_S* z_cap,
			fixed_type pzx_fp[N_MEAS*N_MEAS],fixed_type zDiff_fp[N_MEAS]);
}
Mat_S R_cal(msmt* msmtinfo);
