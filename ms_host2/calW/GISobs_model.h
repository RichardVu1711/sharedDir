#pragma once
#include "../lib/mat_lib.h"
#include "../lib/global_define.h"
#include "../lib/GISmsmt_prcs.h"

void GISobs_model(fixed_type prtcl_X[NUM_VAR],
				int step, msmt* msmtinfo, fixed_type zCap[N_MEAS]);

