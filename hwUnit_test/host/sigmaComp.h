#include "lib/global_define.h"
#include "lib/mat_lib.h"

void store_sigma(fixed_type sigMat_local[NUM_VAR*NUM_PARTICLES],
				fp_str& sigMat);
extern "C"
{
void sigmaComp(fixed_type Pxx[NUM_VAR],
				fp_str& sigMat,
				fixed_type rnd_data[NUM_PARTICLES*NUM_VAR]);
}
