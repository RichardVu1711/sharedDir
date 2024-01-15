#include "lib/global_define.h"
#include "lib/mat_lib.h"

extern "C"
{
void axis2mm(fp_str& sigMat,
				fixed_type sigMat_DB[NUM_PARTICLES*NUM_VAR]){
		for(int i=0; i < NUM_PARTICLES*NUM_VAR;i++ ){
			sigMat_DB[i] = sigMat.read();
		}
}
}
