#pragma once
#define CL_HPP_CL_1_2_DEFAULT_BUILD
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_ENABLE_PROGRAM_CONSTRUCTION_FROM_ARRAY_COMPATIBILITY 1


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
	IDLE,
	DONE
} exec_status;


class srcObj{
private:
	// essential functions:
	// void init_();

public:
//	smpl_info smpl_phase;
	fixed_type prtcls[NUM_VAR*NUM_PARTICLES];
	fixed_type rndSigma[NUM_VAR*NUM_PARTICLES];
	fixed_type wt[NUM_PARTICLES];
	fixed_type obs[N_OBS*10];

	fixed_type pxx[NUM_VAR*NUM_VAR];
	fixed_type mPxx[NUM_VAR*NUM_VAR];
	fixed_type rndrk4[NUM_VAR*4];

	fixed_type state[NUM_VAR];
	fixed_type cAvg[N_MEAS];
	fixed_type nAvg[N_MEAS];


	uint8_t srcIdx;
	unsigned int n_obs=0;
	unsigned int n_tdoa=0;
	unsigned int n_aoa=0;
	// random number for resampling
	fixed_type r_rsmpl=0;
	unsigned int iter_idx=0;
	srcObj(){

	}	//dummy constructor
	srcObj(std::string obs_path,
			uint8_t idx,
			cl::Context& context,
			cl::CommandQueue& q);
	exec_status src_state = IDLE;

};


