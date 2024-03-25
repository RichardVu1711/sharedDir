#pragma once
#include "lib/global_define.h"
#include "lib/mat_lib.h"



extern "C"
{
void ESPCrtParticles(fixed_type X_meanpro[NUM_VAR],
					fp_str& sigMat,	// 13*1024
					fp_str& prtcls,
					fp_str& prtcls2);
}
void load_data_crt( Mat* prtcls_out,
					fp_str& prtcls,
					fp_str& prtcls2);

void store_data_crt(fixed_type X_meanpro[NUM_VAR],
					fp_str& sigMat,	// 13*1024 internal stream
					Mat_S* X_meanpro_out,
					Mat* sigMat_out);
