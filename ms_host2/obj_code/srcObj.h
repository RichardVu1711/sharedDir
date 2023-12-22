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

#include "xcl2.hpp"
#include "experimental/xrt_profile.h"


// extern std::vector<cl::Device> devices;
// extern cl::Device device;
// extern cl::Context context;
// extern cl::Program program;
// extern std::vector<cl::Platform> platforms;
// extern cl::CommandQueue q[Q_LEN];

#pragma once
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
}ptrBuff;


typedef struct queue{
	std::vector<cl::Device> devices;
	cl::Device device;
	cl::Context context;
	cl::Program program;
	std::vector<cl::Platform> platforms;
	cl::CommandQueue q[N_SRC];
	int iterations;
	int MCrun;
} queue;

//kernel sigma parameter
typedef struct k_sigma{
	ptrBuff pxxSqrt;
	ptrBuff rndIn;
	ptrBuff	sigMat;
} k_sigma;

//Sampling Data
typedef struct smpl_info{
	k_sigma sigmaInfo;
};


class ESP_PF{
public:
	ESP_PF(int* argc, char*** argv);
	queue esp_control;
	cl::Kernel kPFU;
	cl::Kernel kCal;
	cl::Kernel kMPxx;
	cl::Kernel kSigma;
	cl::Kernel kCreate;
};

class srcObj{
private:
	// essential functions:
	void init_();
	
public:
//	srcObj(std::string obs_path);
	std::string obs_path;
//	fixed_type state[NUM_VAR];
//	fixed_type pxx[NUM_VAR];
//	fixed_type prtcls[NUM_PARTICLES*NUM_VAR];
//	fixed_type wt[NUM_PARTICLES*1];
	fixed_type obs[N_OBS*10];
	msmt msmtinfo;	// data measurement
	int n_prtcls; // the number of particles that is running on PL
	status excute_status;
	smpl_info smpl_phase;

};


