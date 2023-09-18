#pragma once
#include "lib/global_define.h"
#include "lib/mat_lib.h"


extern "C"
{
void ESPCrtParticles(fixed_type X_meanpro[NUM_VAR],
					fixed_type sigMat[NUM_VAR*NUM_PARTICLES],
					fixed_type prtcls[NUM_VAR*NUM_PARTICLES]);
}
void load_data_crt( Mat_S* prtcls_out,
					fixed_type prtcls[NUM_VAR*NUM_PARTICLES]);

void store_data_crt(fixed_type X_meanpro[NUM_VAR],
					fixed_type sigMat[NUM_VAR*NUM_PARTICLES],
					Mat_S* X_meanpro_out,
					Mat* sigMat_out);
