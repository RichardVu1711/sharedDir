#pragma once
#define CL_HPP_CL_1_2_DEFAULT_BUILD
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_ENABLE_PROGRAM_CONSTRUCTION_FROM_ARRAY_COMPATIBILITY 1


#define N_SRC 1
#define N_OBS 52

#include "../global_define/global_define.h"
#include "../global_define/GISmsmt_prcs.h"
#include "../global_define/mat_lib.h"
#include "../global_define/read_write_csv.h"

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

extern size_t size_Mat;
extern size_t size_Mat_S;
extern size_t size_wt;

extern size_t size_pxx;	// NUM_VAR*NUM_VAR -- 13 x 13
extern size_t size_large; // NUM_VAR*NUM_PARTICLES -- 13 x 1024
extern size_t size_state; // NUM_VAR * 1 -- 13x1

extern size_t size_msmt;
extern size_t size_Rmat;

extern size_t size_zDiff;
extern size_t size_pzx;

typedef enum PSPL{
	PS,
	PL
} PSPL;
typedef enum rw_mode{
	RBUF,
	WBUF
} rw_mode;
typedef enum status{
	INIT,
	SAMP,
	CAL,
	RSMPL,
	IDLE
} status;

typedef struct rndCtrl{
	uint8_t isInit;
	int seed;
} rndCtrl;

typedef struct ptrBuff{
	cl::Buffer buf;
	fixed_type* ptr;
	size_t size;
	rw_mode mode;	//mode = 0 => read, mode = 1 => write
	PSPL allo_mode;
}ptrBuff;


//kernel sigma parameter
typedef struct k_sigma{
	ptrBuff pxxSqrt;
	ptrBuff rndIn;
	ptrBuff	sigMat;
} k_sigma;

typedef struct k_rk4{
	ptrBuff stateIn;
	ptrBuff statePro;
} k_rk4;

typedef struct k_espCrt{
	ptrBuff statePro;
	ptrBuff sigMat;
	ptrBuff	prtcls;
} k_espCrt;

//Sampling Data
typedef struct smpl_info{
	k_espCrt espCrtInfo;
	k_sigma sigmaInfo;
	k_rk4 rk4Info;	// this block is allocated under PS
} smpl_info;


class srcObj{
private:
	// essential functions:
	// void init_();

public:
	srcObj(){

	}	//dummy constructor
	srcObj(std::string obs_path,
			cl::Context& context,
			cl::CommandQueue& q);
	void buffLink(ptrBuff* buffer,size_t in_size,
					cl::Context& context,
					cl::CommandQueue& q,
					rw_mode io_mode, PSPL alloc);
	std::string obs_path;
	fixed_type obs[N_OBS*10];
	fixed_type state[NUM_VAR];
	fixed_type pxx[NUM_VAR];
//	fixed_type wt[NUM_PARTICLES*1];
//	fixed_type prtcls[NUM_PARTICLES*NUM_VAR];
	msmt msmtinfo;	// data measurement
	int n_prtcls; // the number of particles that is running on PL
	status excute_status;
	smpl_info smpl_phase;

};


