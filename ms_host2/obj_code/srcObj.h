#pragma once
#define CL_HPP_CL_1_2_DEFAULT_BUILD
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_ENABLE_PROGRAM_CONSTRUCTION_FROM_ARRAY_COMPATIBILITY 1


#define N_SRC 1

#include "../lib/global_define.h"
#include "../lib/GISmsmt_prcs.h"
#include "../lib/mat_lib.h"
#include "../lib/read_write_csv.h"

#include <CL/cl2.hpp>

#include "ESPObj.h"
#include "xcl2.hpp"
#include "experimental/xrt_profile.h"
#include <iostream>

// extern std::vector<cl::Device> devices;
// extern cl::Device device;
// extern cl::Context context;
// extern cl::Program program;
// extern std::vector<cl::Platform> platforms;
// extern cl::CommandQueue q[Q_LEN];
typedef enum exec_status{
	SMPL,
	CALW,
	RSMP,
	IDLE
} exec_status;

class srcObj{
private:
	// essential functions:
	// void init_();

public:
//	smpl_info smpl_phase;
	fixed_type prtcls[NUM_VAR*NUM_PARTICLES];
//	fixed_type sigma[NUM_VAR*NUM_PARTICLES];
	fixed_type rndSigma[NUM_VAR*NUM_PARTICLES];
	fixed_type rndrk4[NUM_VAR*4];
	fixed_type mPxx[NUM_VAR*NUM_VAR];

//	fixed_type state_pro[NUM_VAR];

	fixed_type obs[N_OBS*10];
	fixed_type state[NUM_VAR];
	fixed_type pxx[NUM_VAR*NUM_VAR];
	uint8_t srcIdx;

	srcObj(){

	}	//dummy constructor
	srcObj(std::string obs_path,
			uint8_t idx,
			cl::Context& context,
			cl::CommandQueue& q);
	exec_status src_state = IDLE;
//	fixed_type prtcls[NUM_PARTICLES*NUM_VAR];
//	fixed_type wt[NUM_PARTICLES*1];
//	msmt msmtinfo;	// data measurement
//	int n_prtcls; // the number of particles that is running on PL
//	status excute_status;

};


