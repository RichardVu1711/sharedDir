#include "lib/global_define.h"
#include "lib/mat_lib.h"

void store_sigma(fixed_type sigMat_local[NUM_VAR*NUM_PARTICLES],
				fixed_type sigMat[NUM_VAR*NUM_PARTICLES]);
extern "C"
{
void sigmaComp(fixed_type Pxx[NUM_VAR],
				fixed_type sigMat[NUM_VAR*NUM_PARTICLES],
				fixed_type rnd_data[NUM_PARTICLES*NUM_VAR]);
}
