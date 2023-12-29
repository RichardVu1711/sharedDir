#include "env_host.h"

int kernel_config(ESP_PF &imp, srcObj& srcX){
	// program kernel

	cl_int err;
	// check if the buffer mode, if it is PL then the Block is on PL
	if(srcX.smpl_phase.sigmaInfo.pxxSqrt.allo_mode == PL)
		OCL_CHECK(err, imp.kSigma = cl::Kernel(imp.esp_control.program,"sigmaComp", &err));

//    OCL_CHECK(err, cl::Kernel kCal = cl::Kernel(program,"CalPzxZdiff", &err));
//    OCL_CHECK(err, cl::Kernel k_mPxx = cl::Kernel(program,"mean_Pxx", &err));
//    OCL_CHECK(err, cl::Kernel kSigma = cl::Kernel(program,"sigmaComp", &err));
//    OCL_CHECK(err, cl::Kernel kCreate = cl::Kernel(program,"ESPCrtParticles", &err));

}
